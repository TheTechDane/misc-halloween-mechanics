/*
 * Dragon Tail Prop v2.4 - XIAO ESP32-C3 Firmware
 * (MP3 + Hysteresis Debugging > 3% Change + Max Speed 50%)
 * 
 * Hardware Pin Mapping:
 * - Potentiometer Wiper : D2 (GPIO4)
 * - Servo Signal Wire   : D8 (GPIO8)
 * - I2S LRC / WS        : D3 (GPIO5)
 * - I2S BCLK            : D4 (GPIO6)
 * - I2S DOUT            : D5 (GPIO7)
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "LittleFS.h"

#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Hardware Pin Definitions
const int POT_PIN   = D2; // GPIO4
const int SERVO_PIN = D8; // GPIO8

#define I2S_LRC   D3      // GPIO5
#define I2S_BCLK  D4      // GPIO6
#define I2S_DOUT  D5      // GPIO7

// --- EASY USER CONFIGURATION ---
const int MAX_SPEED_PERCENT = 50; // Max hastighed sat til 50%

// --- HARDWARE CALIBRATION ---
const int MIN_ADC    = 50;
const int CENTER_ADC = 2048;
const int MAX_ADC    = 4040;
const int STOP_PULSE = 1500;
const int DEADBAND   = 50;

// Automatic conversion from percent to pulse offset (0 to 500us offset)
const int MAX_OFFSET = (MAX_SPEED_PERCENT * 500) / 100;

// Global Objects
AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceLittleFS *file = NULL;
AudioOutputI2S *out = NULL;

Servo myServo;

const char* SOUND_FOLDER = "/sounds";
File currentDir;

void playNextMp3InFolder() {
  if (mp3 && mp3->isRunning()) {
    mp3->stop();
  }

  if (!currentDir) {
    currentDir = LittleFS.open(SOUND_FOLDER);
    if (!currentDir || !currentDir.isDirectory()) {
      Serial.println("[ERROR] Could not open /sounds directory! Check LittleFS upload.");
      return;
    }
  }

  File nextFile = currentDir.openNextFile();

  if (!nextFile) {
    Serial.println("[AUDIO] End of /sounds folder reached. Rewinding...");
    currentDir.rewindDirectory();
    nextFile = currentDir.openNextFile();
  }

  while (nextFile) {
    String name = String(nextFile.name());
    
    if (!nextFile.isDirectory() && name.endsWith(".mp3")) {
      String fullPath = String(SOUND_FOLDER) + "/" + name;
      Serial.print("[AUDIO] Playing file: ");
      Serial.println(fullPath);

      if (file) delete file;
      file = new AudioFileSourceLittleFS(fullPath.c_str());
      
      if (!mp3->begin(file, out)) {
        Serial.println("[ERROR] Failed to start MP3 playback!");
      }
      
      nextFile.close();
      return;
    }
    nextFile.close();
    nextFile = currentDir.openNextFile();
  }
  
  Serial.println("[WARNING] No .mp3 files found in /sounds folder.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Dragon Tail Prop Controller v2.4 (ESP32-C3) ===");

  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR] LittleFS mount failed!");
    return;
  }
  Serial.println("[OK] LittleFS mounted.");

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  out->SetGain(1.0);
  
  mp3 = new AudioGeneratorMP3();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(POT_PIN, INPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 1000, 2000);
  myServo.writeMicroseconds(STOP_PULSE);

  Serial.println("[OK] Hardware initialized. Starting audio loop...");

  playNextMp3InFolder();
}

void loop() {
  // 1. Service I2S Audio Buffers
  if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("[AUDIO] Track finished. Loading next...");
      playNextMp3InFolder();
    }
  }

  // 2. Non-blocking Servo Logic & Conditional Debugging (Hver 20ms = 50Hz)
  static unsigned long lastServoTime = 0;
  static int lastReportedPercent = -999;

  if (millis() - lastServoTime >= 20) {
    lastServoTime = millis();

    int rawADC = analogRead(POT_PIN);
    int targetPulse = STOP_PULSE;
    int speedPercent = 0;

    if (abs(rawADC - CENTER_ADC) <= DEADBAND) {
      targetPulse = STOP_PULSE;
      speedPercent = 0;
    } else if (rawADC < CENTER_ADC) {
      int clamped = constrain(rawADC, MIN_ADC, CENTER_ADC - DEADBAND);
      int offset = map(clamped, MIN_ADC, CENTER_ADC - DEADBAND, -MAX_OFFSET, 0);
      targetPulse = STOP_PULSE + offset;
      
      speedPercent = map(offset, -MAX_OFFSET, 0, -MAX_SPEED_PERCENT, 0);
    } else {
      int clamped = constrain(rawADC, CENTER_ADC + DEADBAND, MAX_ADC);
      int offset = map(clamped, CENTER_ADC + DEADBAND, MAX_ADC, 0, MAX_OFFSET);
      targetPulse = STOP_PULSE + offset;
      
      speedPercent = map(offset, 0, MAX_OFFSET, 0, MAX_SPEED_PERCENT);
    }

    myServo.writeMicroseconds(targetPulse);

    if (abs(speedPercent - lastReportedPercent) > 3) {
      lastReportedPercent = speedPercent;

      Serial.print("[POT CHANGED >3%] Speed: ");
      if (speedPercent > 0) Serial.print("+");
      Serial.print(speedPercent);
      Serial.print("% | ADC Raw: ");
      Serial.print(rawADC);
      Serial.print(" | Pulse: ");
      Serial.print(targetPulse);
      Serial.print(" us");

      if (speedPercent == 0) {
        Serial.println(" (STOPPED / DEADBAND)");
      } else if (speedPercent < 0) {
        Serial.println(" (REVERSE / CCW)");
      } else {
        Serial.println(" (FORWARD / CW)");
      }
    }
  }
}