/*
 * ATtiny412 WS2812B Running Lava (Middle Ground Palette, One-Direction, 30ms & Button Brightness)
 * -----------------------------------------------------------------------------
 * Hardware Pinout (ATtiny412 SOIC-8):
 * - PA2 (Pin 5) : Push Button Input (Active LOW, Internal Pull-Up)
 * - PA3 (Pin 7) : WS2812B Data Output
 * 
 * Button Controls:
 * - Hold Button (>= 500ms) : Continuously cycles brightness up/down in 10% steps
 */

#include <tinyNeoPixel_Static.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define NUM_LEDS     14
#define LEDSTRIP_PIN PIN_PA3  // Physical Pin 7
#define BUTTON_PIN   PIN_PA2  // Physical Pin 5

// EEPROM Storage Configuration
const uint8_t EEPROM_ADDR = 0;
const unsigned long EEPROM_SAVE_DELAY_MS = 10000;

// =============================================================================
// BRIGHTNESS TABLE (PROGMEM)
// =============================================================================
struct BrightnessLevel {
  uint8_t scaler;     // 0-255 scale
  uint8_t percentage; // Display %
};

const BrightnessLevel BRIGHTNESS_TABLE[] PROGMEM = {
  {255, 100}, {230, 90}, {204, 80}, {178, 70}, {153, 60},
  {128, 50},  {102, 40}, {76,  30}, {51,  20}, {25,  10}, {13, 5}
};
const uint8_t TOTAL_BRIGHTNESS_LEVELS = sizeof(BRIGHTNESS_TABLE) / sizeof(BRIGHTNESS_TABLE[0]);

int8_t currentBrightnessIdx = 0;
int8_t brightnessDirection  = -1; // -1 = dim down, +1 = brighten up

// Deferred EEPROM save state
bool savePending = false;
unsigned long lastStateChangeTime = 0;

// Memory buffer for static pixel allocation
uint8_t pixels[NUM_LEDS * 3];
tinyNeoPixel strip = tinyNeoPixel(NUM_LEDS, LEDSTRIP_PIN, NEO_GRB + NEO_KHZ800, pixels);

// =============================================================================
// BUTTON DEBOUNCE & REPEAT MANAGEMENT
// =============================================================================
const uint8_t DEBOUNCE_DELAY_MS        = 50;   // Debounce filter
const uint16_t LONG_PRESS_THRESHOLD    = 500;  // Hold threshold (ms)
const uint16_t HOLD_REPEAT_INTERVAL_MS = 300;  // Hold step interval (ms)

bool lastButtonState       = HIGH;
bool currentButtonState    = HIGH;
unsigned long lastDebounceTime   = 0;
unsigned long buttonPressTime    = 0;
unsigned long lastHoldRepeatTime = 0;
bool isHolding             = false;

// =============================================================================
// LAVA ANIMATION TIMING
// =============================================================================
uint16_t lavaOffset = 0;
unsigned long lastLavaUpdate = 0;

const uint8_t LAVA_SPEED_MS = 30; // Hastighed sat til 30 ms
const uint8_t STEP_SIZE     = 1;  // Trin-størrelse for envejs-bevægelse

// =============================================================================
// EEPROM PERSISTENCE HELPERS
// =============================================================================
void loadSettingsFromEEPROM() {
  uint8_t stored = eeprom_read_byte((uint8_t*)EEPROM_ADDR);
  if (stored != 0xFF) {
    currentBrightnessIdx = (stored >> 4) & 0x0F;
    if (currentBrightnessIdx >= TOTAL_BRIGHTNESS_LEVELS) currentBrightnessIdx = 0;
  }
}

void processDeferredEEPROMSave() {
  if (savePending && (millis() - lastStateChangeTime >= EEPROM_SAVE_DELAY_MS)) {
    uint8_t packed = ((uint8_t)currentBrightnessIdx & 0x0F) << 4;
    eeprom_update_byte((uint8_t*)EEPROM_ADDR, packed);
    savePending = false;
  }
}

void markSettingsChanged() {
  savePending = true;
  lastStateChangeTime = millis();
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
}

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
        if (isHolding) {
          brightnessDirection = -brightnessDirection; // Vend retning ved slip
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

// =============================================================================
// MIDDLE GROUND LAVA PALETTE (ORANGE & RED FOCUS)
// =============================================================================
void setLavaPixel(uint8_t index, uint8_t heat, uint8_t scaleByte) {
  uint8_t r = 0, g = 0, b = 0;

  if (heat > 180) {
    // Varmeste toppe: Varm orange-gul (R:255, G:85-115, B:0)
    r = 255;
    g = 85 + ((heat - 180) * 0.4);
    b = 0;
  } else if (heat > 85) {
    // Rindende lava: Dyb, ren orange (R:255, G:15-85)
    r = 255;
    g = (heat - 85) * 0.74;
    b = 0;
  } else if (heat > 10) {
    // Glødende mørkerød lava-bund
    r = (heat - 10) * 1.5;
    g = 0;
    b = 0;
  } else {
    // Slukket (obsidian-skorpe)
    r = 0; g = 0; b = 0;
  }

  // Skalér i forhold til knappens valgte lysstyrke
  r = ((uint16_t)r * scaleByte) >> 8;
  g = ((uint16_t)g * scaleByte) >> 8;
  b = ((uint16_t)b * scaleByte) >> 8;

  strip.setPixelColor(index, r, g, b);
}

void calculateLava() {
  lavaOffset += STEP_SIZE;
  uint8_t scaleByte = pgm_read_byte(&BRIGHTNESS_TABLE[currentBrightnessIdx].scaler);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // Envejs-bølgebevægelse
    uint16_t wave1 = (lavaOffset - (i * 35)) & 255;
    uint16_t wave2 = ((lavaOffset / 2) - (i * 20)) & 255;

    uint8_t val1 = (wave1 < 128) ? wave1 * 2 : (255 - wave1) * 2;
    uint8_t val2 = (wave2 < 128) ? wave2 * 2 : (255 - wave2) * 2;

    uint8_t heat = (val1 + val2) / 2;

    if (heat > 195) heat = 255;
    else if (heat < 40) heat = 0;

    setLavaPixel(i, heat, scaleByte);
  }
  strip.show();
}

void setup() {
  pinMode(LEDSTRIP_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  loadSettingsFromEEPROM();

  strip.begin();
  strip.show();
}

void loop() {
  processButtonInput();

  unsigned long currentMillis = millis();
  if (currentMillis - lastLavaUpdate >= LAVA_SPEED_MS) {
    lastLavaUpdate = currentMillis;
    calculateLava();
  }

  processDeferredEEPROMSave();
}