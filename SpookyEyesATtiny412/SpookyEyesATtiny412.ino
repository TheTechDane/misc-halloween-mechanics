/*
 * ATtiny412 WS2812B Automatic Scene Sequence
 * Version 2.0
 * -----------------------------------------------------------------------------
 * Automatically cycles through 4 scenes:
 * 1. Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
 * 2. Solid Green:   10 seconds (100% brightness)
 * 3. Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
 * 4. Off (Dark):    15 seconds (Strip completely dark)
 * 
 * Hardware Pinout (ATtiny412 SOIC-8):
 * - VCC  (Pin 1) : 3.3V - 5V Power
 * - PA6  (Pin 2) : Serial TX Debug Output (115200 Baud) [If ENABLE_SERIAL is active]
 * - PA7  (Pin 3) : Unused
 * - PA1  (Pin 4) : Unused
 * - PA2  (Pin 5) : Push Button Input (Active LOW, Internal Pull-Up)
 * - PA0  (Pin 6) : UPDI Programming Pin
 * - PA3  (Pin 7) : WS2812B Data Output
 * - GND  (Pin 8) : Ground
 * 
 * Button Controls:
 * - Short Press (< 500ms)  : Cycles Colors
 * - Hold Button (>= 500ms) : Continuously cycles brightness up/down in 10% steps
 * 
 * Persistence:
 * - Deferred EEPROM Auto-Save: Packs Color (4 bits) + Brightness (4 bits) into 1 byte.
 * - Saves 10 seconds after the last button activity to protect EEPROM lifespan.
 */

#include <tinyNeoPixel_Static.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

// =============================================================================
// COMPILER FLAGS & HARDWARE CONFIGURATION
// =============================================================================
#define ENABLE_SERIAL

#define NUM_LEDS     14
#define LEDSTRIP_PIN PIN_PA3  // Physical Pin 7
#define BUTTON_PIN   PIN_PA2  // Physical Pin 5

// EEPROM Storage Address
const uint8_t EEPROM_ADDR = 0;
const unsigned long EEPROM_SAVE_DELAY_MS = 10000; // Delay save 10s after last press

// =============================================================================
// SCENE TIMING CONFIGURATION
// =============================================================================
const unsigned long LIGHT_SCENE_DURATION_MS = 10000; // 10s per active scene
const unsigned long OFF_SCENE_DURATION_MS   = 15000; // 15s for dark scene

const uint8_t MIN_BRIGHTNESS = 26;  // ~10% floor for breathing
const uint8_t MAX_BRIGHTNESS = 255; // Peak brightness

// =============================================================================
// COLOR PALETTE & BRIGHTNESS LEVELS (PROGMEM COMBINED STRUCTS)
// =============================================================================
struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct BrightnessLevel {
  uint8_t scaler;     // 0-255 scale
  uint8_t percentage; // Display %
};

const RGBColor COLOR_PALETTE[] PROGMEM = {
  {0,   255, 0  }, // Green
  {255, 0,   0  }, // Red
  {0,   0,   255}, // Blue
  {255, 147, 41 }, // Warm White
  {0,   255, 255}, // Cyan
  {255, 0,   255}, // Magenta
  {255, 180, 0  }  // Gold
};
const uint8_t TOTAL_COLORS = sizeof(COLOR_PALETTE) / sizeof(COLOR_PALETTE[0]);

const BrightnessLevel BRIGHTNESS_TABLE[] PROGMEM = {
  {255, 100}, {230, 90}, {204, 80}, {178, 70}, {153, 60},
  {128, 50},  {102, 40}, {76,  30}, {51,  20}, {25,  10}, {13, 5}
};
const uint8_t TOTAL_BRIGHTNESS_LEVELS = sizeof(BRIGHTNESS_TABLE) / sizeof(BRIGHTNESS_TABLE[0]);

// State variables
uint8_t currentColorIndex     = 0;
int8_t currentBrightnessIdx   = 0;
int8_t brightnessDirection    = -1; // -1 = dim down, +1 = brighten up

// Deferred EEPROM save state
bool savePending              = false;
unsigned long lastStateChangeTime = 0;

// Memory buffer for static pixel allocation
uint8_t pixels[NUM_LEDS * 3];

// Initialize tinyNeoPixel Static
tinyNeoPixel strip = tinyNeoPixel(NUM_LEDS, LEDSTRIP_PIN, NEO_GRB + NEO_KHZ800, pixels);

// =============================================================================
// BUTTON DEBOUNCE & REPEAT MANAGEMENT
// =============================================================================
const uint8_t DEBOUNCE_DELAY_MS       = 50;  // Debounce filter
const uint16_t LONG_PRESS_THRESHOLD   = 500; // Hold threshold (ms)
const uint16_t HOLD_REPEAT_INTERVAL_MS= 300; // Hold step interval (ms)

bool lastButtonState       = HIGH;
bool currentButtonState    = HIGH;
unsigned long lastDebounceTime   = 0;
unsigned long buttonPressTime    = 0;
unsigned long lastHoldRepeatTime = 0;
bool isHolding             = false;

// Dynamic Scene Control States
enum SceneState : uint8_t {
  SCENE_1_BREATHE,
  SCENE_2_SOLID,
  SCENE_3_BREATHE,
  SCENE_4_OFF
};

SceneState currentScene       = SCENE_1_BREATHE;
unsigned long sceneStartTime  = 0;

// Breathing animation state
uint8_t currentBreatheLevel   = MIN_BRIGHTNESS;
int8_t breatheDirection       = 1;
unsigned long lastBreatheStepTime = 0;
const uint8_t BREATHE_STEP_INTERVAL_MS = 4;

// Helper to fetch percentage from PROGMEM
inline uint8_t getBrightnessPercent(uint8_t idx) {
  return pgm_read_byte(&BRIGHTNESS_TABLE[idx].percentage);
}

// =============================================================================
// EEPROM PERSISTENCE HELPERS
// =============================================================================
inline uint8_t packSettings() {
  return (((uint8_t)currentBrightnessIdx & 0x0F) << 4) | (currentColorIndex & 0x0F);
}

void unpackSettings(uint8_t data) {
  currentColorIndex    = data & 0x0F;
  currentBrightnessIdx = (data >> 4) & 0x0F;

  if (currentColorIndex >= TOTAL_COLORS) currentColorIndex = 0;
  if (currentBrightnessIdx >= TOTAL_BRIGHTNESS_LEVELS) currentBrightnessIdx = 0;
}

void printStateDebug(const __FlashStringHelper* label) {
#ifdef ENABLE_SERIAL
  Serial.print(label);
  Serial.print(F(" C:"));
  Serial.print(currentColorIndex);
  Serial.print(F(" B:"));
  Serial.print(getBrightnessPercent(currentBrightnessIdx));
  Serial.println(F("%"));
#endif
}

void loadSettingsFromEEPROM() {
  uint8_t stored = eeprom_read_byte((uint8_t*)EEPROM_ADDR);
  if (stored != 0xFF) {
    unpackSettings(stored);
    printStateDebug(F("LOAD"));
  }
}

void processDeferredEEPROMSave() {
  if (savePending && (millis() - lastStateChangeTime >= EEPROM_SAVE_DELAY_MS)) {
    uint8_t packed = packSettings();
    eeprom_update_byte((uint8_t*)EEPROM_ADDR, packed);
    savePending = false;
    printStateDebug(F("[EEPROM SAVE]"));
  }
}

void markSettingsChanged() {
  savePending = true;
  lastStateChangeTime = millis();
}

// Optimized integer scaling helper (Pure Bit Shifts)
inline uint8_t scaleChannel(uint8_t colorVal, uint8_t intensity, uint8_t scaleByte) {
  uint16_t step1 = ((uint16_t)colorVal * intensity) >> 8;
  return (uint16_t)(step1 * scaleByte) >> 8;
}

// Helper function to update NeoPixel strip
void setStripColor(uint8_t intensity) {
  uint8_t scaleByte = pgm_read_byte(&BRIGHTNESS_TABLE[currentBrightnessIdx].scaler);
  
  RGBColor activeColor;
  activeColor.r = pgm_read_byte(&COLOR_PALETTE[currentColorIndex].r);
  activeColor.g = pgm_read_byte(&COLOR_PALETTE[currentColorIndex].g);
  activeColor.b = pgm_read_byte(&COLOR_PALETTE[currentColorIndex].b);

  uint8_t r = scaleChannel(activeColor.r, intensity, scaleByte);
  uint8_t g = scaleChannel(activeColor.g, intensity, scaleByte);
  uint8_t b = scaleChannel(activeColor.b, intensity, scaleByte);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

// Handler for cycling colors on short press
void handleShortPress() {
  currentColorIndex = (currentColorIndex + 1) % TOTAL_COLORS;
  markSettingsChanged();
  printStateDebug(F("COLOR"));
}

// Handler for stepping brightness up/down during hold
void stepBrightness() {
  int8_t nextIdx = currentBrightnessIdx + brightnessDirection;

  if (nextIdx >= TOTAL_BRIGHTNESS_LEVELS) {
    currentBrightnessIdx = TOTAL_BRIGHTNESS_LEVELS - 1;
    brightnessDirection = -1;
  } else if (nextIdx < 0) {
    currentBrightnessIdx = 0;
    brightnessDirection = 1;
  } else {
    currentBrightnessIdx = nextIdx;
  }

  markSettingsChanged();
  printStateDebug(F("BRIGHT"));
}

// Read and process button input
void processButtonInput() {
  bool reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != currentButtonState) {
      currentButtonState = reading;

      if (currentButtonState == LOW) {
        buttonPressTime = now;
        isHolding = false;
      } else {
        if (!isHolding && (now - buttonPressTime < LONG_PRESS_THRESHOLD)) {
          handleShortPress();
        } else if (isHolding) {
          brightnessDirection = -brightnessDirection;
#ifdef ENABLE_SERIAL
          Serial.print(F("DIR:"));
          Serial.println(brightnessDirection > 0 ? F("UP") : F("DN"));
#endif
        }
      }
    }

    if (currentButtonState == LOW) {
      if (!isHolding && (now - buttonPressTime >= LONG_PRESS_THRESHOLD)) {
        isHolding = true;
        lastHoldRepeatTime = now;
        stepBrightness();
      } else if (isHolding && (now - lastHoldRepeatTime >= HOLD_REPEAT_INTERVAL_MS)) {
        lastHoldRepeatTime = now;
        stepBrightness();
      }
    }
  }

  lastButtonState = reading;
}

// Update LEDs based on current scene state using non-blocking timing
void updateSceneMachine() {
  unsigned long currentMillis = millis();

  switch (currentScene) {
    case SCENE_1_BREATHE:
      if (currentMillis - sceneStartTime >= LIGHT_SCENE_DURATION_MS) {
        currentScene = SCENE_2_SOLID;
        sceneStartTime = currentMillis;
#ifdef ENABLE_SERIAL
        Serial.println(F("SCENE:2"));
#endif
      } else {
        if (currentMillis - lastBreatheStepTime >= BREATHE_STEP_INTERVAL_MS) {
          lastBreatheStepTime = currentMillis;
          int16_t nextLevel = currentBreatheLevel + breatheDirection;

          if (nextLevel >= MAX_BRIGHTNESS) {
            currentBreatheLevel = MAX_BRIGHTNESS;
            breatheDirection = -1;
          } else if (nextLevel <= MIN_BRIGHTNESS) {
            currentBreatheLevel = MIN_BRIGHTNESS;
            breatheDirection = 1;
          } else {
            currentBreatheLevel = (uint8_t)nextLevel;
          }
        }
        setStripColor(currentBreatheLevel);
      }
      break;

    case SCENE_2_SOLID:
      if (currentMillis - sceneStartTime >= LIGHT_SCENE_DURATION_MS) {
        currentScene = SCENE_3_BREATHE;
        sceneStartTime = currentMillis;
#ifdef ENABLE_SERIAL
        Serial.println(F("SCENE:3"));
#endif
      } else {
        setStripColor(MAX_BRIGHTNESS);
      }
      break;

    case SCENE_3_BREATHE:
      if (currentMillis - sceneStartTime >= LIGHT_SCENE_DURATION_MS) {
        currentScene = SCENE_4_OFF;
        sceneStartTime = currentMillis;
#ifdef ENABLE_SERIAL
        Serial.println(F("SCENE:4"));
#endif
      } else {
        if (currentMillis - lastBreatheStepTime >= BREATHE_STEP_INTERVAL_MS) {
          lastBreatheStepTime = currentMillis;
          int16_t nextLevel = currentBreatheLevel + breatheDirection;

          if (nextLevel >= MAX_BRIGHTNESS) {
            currentBreatheLevel = MAX_BRIGHTNESS;
            breatheDirection = -1;
          } else if (nextLevel <= MIN_BRIGHTNESS) {
            currentBreatheLevel = MIN_BRIGHTNESS;
            breatheDirection = 1;
          } else {
            currentBreatheLevel = (uint8_t)nextLevel;
          }
        }
        setStripColor(currentBreatheLevel);
      }
      break;

    case SCENE_4_OFF:
      if (currentMillis - sceneStartTime >= OFF_SCENE_DURATION_MS) {
        currentScene = SCENE_1_BREATHE;
        sceneStartTime = currentMillis;
#ifdef ENABLE_SERIAL
        Serial.println(F("SCENE:1"));
#endif
      } else {
        setStripColor(0);
      }
      break;
  }
}

void setup() {
#ifdef ENABLE_SERIAL
  Serial.begin(115200, SERIAL_8N1 | SERIAL_TX_ONLY);
  Serial.println(F("READY"));
#endif

  pinMode(LEDSTRIP_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  loadSettingsFromEEPROM();

  strip.begin();
  strip.show();

  sceneStartTime = millis();
}

void loop() {
  processButtonInput();
  updateSceneMachine();
  processDeferredEEPROMSave();
}