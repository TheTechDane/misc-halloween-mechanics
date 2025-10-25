//================================================================
//  JURASSIC PARK ANIMATRONIC RIG (ESP32-C3)
//
//  FINAL NON-BLOCKING VERSION (v12.3 - True Random Fix)
//
//  - Board: ESP32-C3 Super Mini
//  - Serial: Uses Serial1 (pins 20, 21)
//  - Flashlight: 4
//  - Servo: 2
//  - BUSY Pin: 1 (from DFPlayer pin 16)
//
//  *** MODIFIED ***
//  - Uses esp_random() for a true random seed (FIXES PEEK BUG)
//  - LED_BUILTIN logic is inverted (active-LOW)
//  - Added debug Serial.print to random timers
//
//  WIRING ASSUMPTIONS:
//  - ESP32 Pin 21 (TX1) -> 1K Resistor -> DFPlayer RX
//  - ESP32 Pin 20 (RX1) <- 1K Resistor <- DFPlayer TX
//  - ESP32 Pin 1        <- DFPlayer BUSY (Pin 16)
//
//  Libraries Needed:
//  - DFRobotDFPlayerMini: by DFRobot
//  - ESP32Servo: by Kevin Harrington / John K. H.
//================================================================

// --- 1. INCLUDE LIBRARIES ---
#include "DFRobotDFPlayerMini.h"
#include "ESP32Servo.h"
#include "HardwareSerial.h"

// --- 2. DEFINE PINS & CONSTANTS ---

// Flashlight (MOSFET Gate)
const int FLASHLIGHT_PIN = 4;

// Internal LED Pin
const int LED_BUILTIN_PIN = LED_BUILTIN; 

// Servo (Signal Pin)
const int SERVO_PIN = 2;
const int SERVO_POS_HIDE = 0;   // Position: T-Rex is hidden
const int SERVO_POS_POPUP = 80; // Position: T-Rex pops up
const int SERVO_POS_PEEK = 30;  // Position: T-Rex "passes by"

// Servo Speed Control
const int SERVO_SWEEP_DELAY_MS = 30; // 30ms/degree

// DFPlayer (using Serial Port 1 on custom pins)
const int DFPLAYER_RX_PIN = 20; // ESP32 RX1 (connects to DFPlayer TX)
const int DFPLAYER_TX_PIN = 21; // ESP32 TX1 (connects to DFPlayer RX)
const int BUSY_PIN = 1;         // Connect to DFPlayer 'BUSY' (Pin 16)

// Sound File Definitions (Aligned)
const int SOUND_MAIN_SEQUENCE = 6; // e.g., 0006.mp3 (the big roar)
const int SOUND_PEEK = 1;          // e.g., 0001.mp3 (short "pass-by" sound)
const int SOUND_AMBIENT = 2;       // e.g., 0002.mp3 (long ambient jungle loop)

// --- 3. CREATE OBJECTS ---
Servo myServo;
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial mySerial(1); 

// --- 4. STATE MACHINE VARIABLES ---

// ----- Main Sequence -----
enum MainState {
  STATE_IDLE,
  STATE_RUNNING_SEQUENCE
};
MainState currentState = STATE_IDLE;
int sequenceStep = 0;
unsigned long nextEventTime = 0;
unsigned long nextStepTime = 0;
int currentServoPos = SERVO_POS_HIDE;

// ----- Random Peek Sequence -----
enum PeekState {
  PEEK_IDLE,
  PEEK_WAITING
};
PeekState currentPeekState = PEEK_IDLE;
unsigned long nextPeekEventTime = 0;
unsigned long peekStepTimer = 0;

// ----- BUSY Pin Logic Tracker -----
unsigned long lastSoundTriggerTime = 0;
// Wait 1 second after sound finishes before restarting ambient
const long AMBIENT_RETRIGGER_DELAY = 1000; 


// --- 5. SETUP FUNCTION (runs once) ---
void setup() {
  Serial.begin(115200);
  Serial.println("Jurassic Park Rig Initializing (v12.3 - True Random Fix)...");

  // Init random number generator
  // *** MODIFIED *** Use the ESP32's built-in hardware True Random Number Generator
  randomSeed(esp_random()); 
  Serial.println("True random seed set.");

  // --- Initialize BUSY Pin ---
  pinMode(BUSY_PIN, INPUT_PULLUP);
  Serial.println("BUSY pin initialized.");

  // --- Initialize Servo ---
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_POS_HIDE);
  currentServoPos = SERVO_POS_HIDE;
  Serial.println("Servo attached.");

  // --- Initialize Flashlight & Internal LED ---
  pinMode(FLASHLIGHT_PIN, OUTPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT); 
  digitalWrite(FLASHLIGHT_PIN, LOW);
  digitalWrite(LED_BUILTIN_PIN, HIGH); // HIGH = LED OFF
  Serial.println("Flashlight and Internal LED initialized.");

  // --- Initialize DFPlayer ---
  mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); 
  Serial.println("Initializing DFPlayer on Serial1...");

  // Set ACK to 'false' as requested
  if (!myDFPlayer.begin(mySerial, false, false)) { 
    Serial.println("!!! Unable to begin DFPlayer. Check wiring. Halting. !!!");
    while (true); // Halt
  }
  
  Serial.println("DFPlayer Mini online.");
  myDFPlayer.volume(25); // Set volume (0-30)
  // Play the ambient track (2) briefly to "wake up"
  myDFPlayer.play(SOUND_AMBIENT); 
  delay(500);            
  myDFPlayer.stop();

  // Schedule the FIRST events
  setNextRandomEventTime();
  setNextRandomPeekTime();
  
  // Initialize the sound timer
  lastSoundTriggerTime = millis();
  
  Serial.println("Setup complete. Running loop...");
}

// --- 6. MAIN LOOP (runs thousands of times per second) ---
void loop() {
  // Check for any messages from the DFPlayer (errors, etc.)
  checkDFPlayer(); 

  // These functions run constantly, checking their timers.
  // They do NOT block each other.
  
  handleMainSequence();
  handleRandomPeek();
  
  // Handle the idle ambient sound
  handleAmbientSound();
}

// --- 7. HELPER FUNCTIONS ---

/**
 * @brief Checks for and prints messages from the DFPlayer (errors, etc.)
 */
void checkDFPlayer() {
  if (myDFPlayer.available()) {
    // Read the type of message
    uint8_t type = myDFPlayer.readType(); 

    Serial.print("DFPlayer Msg: ");
    switch (type) {
      case TimeOut:
        Serial.println("Time Out!");
        break;
      case WrongStack:
        Serial.println("Stack Wrong!");
        break;
      case DFPlayerCardInserted:
        Serial.println("Card Inserted!");
        break;
      case DFPlayerCardRemoved:
        Serial.println("Card Removed!");
        break;
      case DFPlayerCardOnline:
        Serial.println("Card Online!");
        break;
      case DFPlayerPlayFinished:
        Serial.print("Track Finished, file: ");
        Serial.println(myDFPlayer.read());
        break;
      case DFPlayerError:
        Serial.print("DFPlayer Error: ");
        switch (myDFPlayer.read()) {
          case Busy: Serial.println("Busy"); break;
          case Sleeping: Serial.println("Sleeping"); break;
          case SerialWrongStack: Serial.println("Serial Wrong Stack"); break;
          case CheckSumNotMatch: Serial.println("Check Sum Not Match"); break;
          case FileIndexOut: Serial.println("File Index Out"); break;
          case FileMismatch: Serial.println("File Mismatch"); break;
          case Advertise: Serial.println("Advertise"); break;
          default: break;
        }
        break;
      default:
        break;
    }
  }
}

/**
 * @brief Restarts ambient sound using BUSY pin.
 */
void handleAmbientSound() {
  // Only run if we are in a fully idle state
  if (currentState == STATE_IDLE && currentPeekState == PEEK_IDLE) {
    
    // Check if the DFPlayer is idle (BUSY pin is HIGH)
    if (digitalRead(BUSY_PIN) == HIGH) {
      
      // Player is idle. Check if it's been idle for our re-trigger delay.
      if (millis() - lastSoundTriggerTime > AMBIENT_RETRIGGER_DELAY) {
        Serial.print(">>> Player idle. Starting Ambient Track ");
        Serial.println(SOUND_AMBIENT);
        
        // Play the track ONCE.
        myDFPlayer.play(SOUND_AMBIENT); 
        
        // Reset the timer so we don't spam this command.
        lastSoundTriggerTime = millis();
      }
    } else {
      // Player is busy (playing a sound). Constantly reset the timer.
      // This ensures we wait AMBIENT_RETRIGGER_DELAY *after* it's done.
      lastSoundTriggerTime = millis();
    }
  }
}

/**
 * @brief Manages the main T-Rex pop-up sequence state.
 */
void handleMainSequence() {
  unsigned long currentTime = millis();

  // === Part 1: Check if we are IDLE and it's time to START ===
  if (currentState == STATE_IDLE && currentTime >= nextEventTime) {
    
    Serial.println("===============================");
    Serial.println("=== Main Sequence Triggered ===");
    Serial.println("===============================");
    currentState = STATE_RUNNING_SEQUENCE; 
    sequenceStep = 0;
    nextStepTime = currentTime;
  }

  // === Part 2: Check if we are RUNNING and it's time for the NEXT STEP ===
  if (currentState == STATE_RUNNING_SEQUENCE) {
    if (currentTime >= nextStepTime) {
      runMainSequenceStep();
    }
  }
}

/**
 * @brief This is the "switch" that runs each step of the main sequence.
 */
void runMainSequenceStep() {
  switch (sequenceStep) {
    case 0: // 1. Flashlight turns ON
      Serial.println("  1. Flashlight ON");
      digitalWrite(FLASHLIGHT_PIN, HIGH);
      digitalWrite(LED_BUILTIN_PIN, LOW); // LOW = LED ON
      nextStepTime = millis() + 1000;
      sequenceStep = 1;
      break;

    case 1: // 3. Flashlight turns OFF
      Serial.println("  3. Flashlight OFF");
      digitalWrite(FLASHLIGHT_PIN, LOW);
      digitalWrite(LED_BUILTIN_PIN, HIGH); // HIGH = LED OFF
      nextStepTime = millis() + 500;
      sequenceStep = 2;
      break;

    case 2: // 5. Flashlight turns ON
      Serial.println("  5. Flashlight ON (quick flash)");
      digitalWrite(FLASHLIGHT_PIN, HIGH);
      digitalWrite(LED_BUILTIN_PIN, LOW); // LOW = LED ON
      nextStepTime = millis() + 500;
      sequenceStep = 3;
      break;

    case 3: // 7. Flashlight turns OFF
      Serial.println("  7. Flashlight OFF");
      digitalWrite(FLASHLIGHT_PIN, LOW);
      digitalWrite(LED_BUILTIN_PIN, HIGH); // HIGH = LED OFF
      nextStepTime = millis() + 2000;
      sequenceStep = 4;
      break;

    case 4: // 9. Play Main Sequence Sound
      Serial.print("  9. Playing Main Sequence Sound (Track ");
      Serial.print(SOUND_MAIN_SEQUENCE);
      Serial.println(")");
      myDFPlayer.play(SOUND_MAIN_SEQUENCE); // Plays main track (6)
      lastSoundTriggerTime = millis(); // Reset ambient timer
      nextStepTime = millis() + 1000;
      sequenceStep = 5;
      break;

    case 5: // 11. Flashlight ON (for the reveal)
      Serial.println("  11. Flashlight ON (Reveal)");
      digitalWrite(FLASHLIGHT_PIN, HIGH);
      digitalWrite(LED_BUILTIN_PIN, LOW); // LOW = LED ON
      currentServoPos = SERVO_POS_HIDE; 
      myServo.write(currentServoPos);
      nextStepTime = millis() + 500;
      sequenceStep = 6;
      break;

    case 6: // 12. SLOW SERVO SWEEP UP (to 80 degrees)
      if (currentServoPos == SERVO_POS_HIDE) {
        Serial.println("  12. T-Rex slowly rising...");
      }
      if (currentServoPos < SERVO_POS_POPUP) { // Sweeps to 80
        currentServoPos++; // Move 1 degree
        myServo.write(currentServoPos);
        nextStepTime = millis() + SERVO_SWEEP_DELAY_MS;
      } else {
        Serial.println("  13. T-Rex is fully visible.");
        nextStepTime = millis() + 2000;
        sequenceStep = 7;
      }
      break;

    case 7: // 14. HIDE T-REX
      Serial.println("  14. Hiding! (Flashlight OFF, Servo HIDE)");
      digitalWrite(FLASHLIGHT_PIN, LOW);
      digitalWrite(LED_BUILTIN_PIN, HIGH); // HIGH = LED OFF
      myServo.write(SERVO_POS_HIDE);
      currentServoPos = SERVO_POS_HIDE;
      nextStepTime = millis() + 1500;
      sequenceStep = 8;
      break;

    case 8: // 15. Sequence is OVER
      Serial.println("=== Main Sequence Complete ===");
      currentState = STATE_IDLE;
      setNextRandomEventTime();
      // Ambient sound will take over via BUSY pin
      break;
  }
}

/**
 * @brief Manages the separate "pass-by" or "peek" event.
 */
void handleRandomPeek() {
  // CRITICAL: Do NOT start a peek if the main sequence is running!
  if (currentState == STATE_RUNNING_SEQUENCE) {
    return;
  }

  unsigned long currentTime = millis();

  // === Part 1: Check if we are IDLE and it's time to START ===
  if (currentPeekState == PEEK_IDLE && currentTime >= nextPeekEventTime) {

    Serial.println("--- Random Peek Start ---");
    
    // Play the "peek" sound
    Serial.print("    -> Playing Peek Sound (Track ");
    Serial.print(SOUND_PEEK);
    Serial.println(")");
    myDFPlayer.play(SOUND_PEEK); // Play peek track (1)
    lastSoundTriggerTime = millis(); // Reset ambient timer
    
    // Turn on light and move servo (to 30 degrees)
    digitalWrite(FLASHLIGHT_PIN, HIGH);
    digitalWrite(LED_BUILTIN_PIN, LOW); // LOW = LED ON
    myServo.write(SERVO_POS_PEEK);

    currentPeekState = PEEK_WAITING; // Change state
    peekStepTimer = currentTime + 1500; // Set timer for 1.5s
  }

  // === Part 2: Check if we are WAITING and it's time to END ===
  if (currentPeekState == PEEK_WAITING && currentTime >= peekStepTimer) {
    Serial.println("--- Random Peek End ---");
    // Turn off light and hide servo
    digitalWrite(FLASHLIGHT_PIN, LOW);
    digitalWrite(LED_BUILTIN_PIN, HIGH); // HIGH = LED OFF
    myServo.write(SERVO_POS_HIDE);
    
    currentPeekState = PEEK_IDLE; // Go back to idle
    setNextRandomPeekTime();      // Schedule the *next* peek
    // Ambient sound will take over via BUSY pin
  }
}

/**
 * @brief Schedules the next main event 20 to 50 seconds from now.
 */
void setNextRandomEventTime() {
  long randomDelay = random(20, 51) * 1000; // 20-50 seconds
  nextEventTime = millis() + randomDelay;
  
  Serial.print("Next main event scheduled in ");
  Serial.print(randomDelay / 1000.0);
  Serial.println(" seconds.");
}

/**
 * @brief Schedules the next peek event 30 to 60 seconds from now.
 */
void setNextRandomPeekTime() {
  long randomDelay = random(30, 61) * 1000; // 30-60 seconds
  nextPeekEventTime = millis() + randomDelay;

  Serial.print("Next random peek scheduled in ");
  Serial.print(randomDelay / 1000.0);
  Serial.println(" seconds.");
}