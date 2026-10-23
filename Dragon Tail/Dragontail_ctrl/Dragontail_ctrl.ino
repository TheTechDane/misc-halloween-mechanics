/*
 * Dragon Tail Prop v2.0 - XIAO ESP32-C3 Compatible Firmware
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "LittleFS.h"

#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

const int POT_PIN   = D2;
const int SERVO_PIN = D8;

#define I2S_LRC   D3
#define I2S_BCLK  D4
#define I2S_DOUT  D5

// --- EASY USER CONFIGURATION ---
const int MAX_SPEED_PERCENT = 60; // 0 to 100%

// --- HARDWARE CALIBRATION ---
const int MIN_ADC    = 50;
const int CENTER_ADC = 2048;
const int MAX_ADC    = 4040;
const int STOP_PULSE = 1500;
const int DEADBAND   = 50;

const int MAX_OFFSET = (MAX_SPEED_PERCENT * 500) / 100;

AudioGeneratorWAV *wav = NULL;
AudioFileSourceLittleFS *file = NULL;
AudioOutputI2S *out = NULL;

Servo myServo;

const char* SOUND_FOLDER = "/sounds";
File currentDir;

void playNextWavInFolder() {
  if (wav && wav->isRunning()) {
    wav->stop();
  }

  if (!currentDir) {
    currentDir = LittleFS.open(SOUND_FOLDER);
    if (!currentDir || !currentDir.isDirectory()) return;
  }

  File nextFile = currentDir.openNextFile();

  if (!nextFile) {
    currentDir.rewindDirectory();
    nextFile = currentDir.openNextFile();
  }

  while (nextFile) {
    String name = String(nextFile.name());
    if (!nextFile.isDirectory() && name.endsWith(".wav")) {
      String fullPath = String(SOUND_FOLDER) + "/" + name;
      if (file) delete file;
      file = new AudioFileSourceLittleFS(fullPath.c_str());
      wav->begin(file, out);
      nextFile.close();
      return;
    }
    nextFile.close();
    nextFile = currentDir.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!LittleFS.begin(true)) return;

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  out->SetGain(0.8);
  
  wav = new AudioGeneratorWAV();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(POT_PIN, INPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 1000, 2000);
  myServo.writeMicroseconds(STOP_PULSE);

  playNextWavInFolder();
}

void loop() {
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      playNextWavInFolder();
    }
  }

  static unsigned long lastServoTime = 0;
  if (millis() - lastServoTime >= 20) {
    lastServoTime = millis();

    int rawADC = analogRead(POT_PIN);
    int targetPulse = STOP_PULSE;

    if (abs(rawADC - CENTER_ADC) <= DEADBAND) {
      targetPulse = STOP_PULSE;
    } else if (rawADC < CENTER_ADC) {
      int clamped = constrain(rawADC, MIN_ADC, CENTER_ADC - DEADBAND);
      int offset = map(clamped, MIN_ADC, CENTER_ADC - DEADBAND, -MAX_OFFSET, 0);
      targetPulse = STOP_PULSE + offset;
    } else {
      int clamped = constrain(rawADC, CENTER_ADC + DEADBAND, MAX_ADC);
      int offset = map(clamped, CENTER_ADC + DEADBAND, MAX_ADC, 0, MAX_OFFSET);
      targetPulse = STOP_PULSE + offset;
    }

    myServo.writeMicroseconds(targetPulse);
  }
}