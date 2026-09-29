/*
 * ATtiny412 WS2812B Automatic Scene Sequence
 * -----------------------------------------------------------------------------
 * Automatically cycles through 4 scenes:
 * 1. Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
 * 2. Solid Green:   10 seconds (100% brightness)
 * 3. Breathe Scene: 10 seconds (10% to 100% brightness, 2s per pulse)
 * 4. Off (Dark):    15 seconds (Strip completely dark)
 * 
 * Hardware Configuration:
 * - PA3 (Pin 7): WS2812B NeoPixel Data Output
 * - PA6 (Pin 2): Serial TX Debug Output (115200 baud, TX-Only) [If Enabled]
 */

#include <tinyNeoPixel_Static.h>

// =============================================================================
// COMPILER FLAGS & HARDWARE CONFIGURATION
// =============================================================================
// Comment out the line below to completely remove Serial code and save Flash memory
#define ENABLE_SERIAL

#define NUM_LEDS 14
#define LEDSTRIP_PIN PIN_PA3  // Physical Pin 7 (WS2812B Data Line)

// =============================================================================
// SCENE TIMING CONFIGURATION (In milliseconds)
// =============================================================================
const unsigned long LIGHT_SCENE_DURATION_MS = 7000; // Duration for Scenes 1, 2, and 3 (7 seconds each)
const unsigned long OFF_SCENE_DURATION_MS   = 15000; // Duration for Scene 4 Dark (15 seconds)

// BRIGHTNESS BOUNDARIES FOR BREATHE SCENE
const uint8_t MIN_BRIGHTNESS = 26;  // ~10% of 255
const uint8_t MAX_BRIGHTNESS = 255; // 100%

// FIXED PULSE SPEED: 2000 ms total (1000 ms up / 1000 ms down)
// 1000 ms / 229 steps (255 - 26) = ~4.36 ms delay per step
const float STEP_DELAY_MS = 1000.0 / (MAX_BRIGHTNESS - MIN_BRIGHTNESS); 

// Memory buffer for static pixel allocation (Saves SRAM)
uint8_t pixels[NUM_LEDS * 3];

// Initialize tinyNeoPixel Static
tinyNeoPixel strip = tinyNeoPixel(NUM_LEDS, LEDSTRIP_PIN, NEO_GRB + NEO_KHZ800, pixels);

// Helper function to set all LEDs to a specific Green brightness (0 to 255)
void setGreenBrightness(uint8_t brightness) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0, brightness, 0); // Green channel is second in GRB
  }
  strip.show();
}

// Helper function to execute the Breathing Effect for a given duration
void runBreatheScene(unsigned long durationMs) {
  unsigned long breatheStartTime = millis();

  while (millis() - breatheStartTime < durationMs) {
    // Pulse Up (10% to 100% over 1000 ms)
    for (int b = MIN_BRIGHTNESS; b <= MAX_BRIGHTNESS; b++) {
      if (millis() - breatheStartTime >= durationMs) break;
      setGreenBrightness(b);
      delayMicroseconds((uint16_t)(STEP_DELAY_MS * 1000));
    }

    // Pulse Down (100% to 10% over 1000 ms)
    for (int b = MAX_BRIGHTNESS; b >= MIN_BRIGHTNESS; b--) {
      if (millis() - breatheStartTime >= durationMs) break;
      setGreenBrightness(b);
      delayMicroseconds((uint16_t)(STEP_DELAY_MS * 1000));
    }
  }
}

void setup() {
#ifdef ENABLE_SERIAL
  Serial.begin(115200, SERIAL_8N1 | SERIAL_TX_ONLY);
  Serial.println(F("ATtiny412 4-Scene Flow Started"));
#endif

  pinMode(LEDSTRIP_PIN, OUTPUT);
  strip.begin();
  strip.show(); // Ensure LEDs start off
}

void loop() {
  // =========================================================================
  // SCENE 1: Breathe (10 seconds)
  // =========================================================================
#ifdef ENABLE_SERIAL
  Serial.println(F("Scene 1: Breathing (10s)..."));
#endif
  runBreatheScene(LIGHT_SCENE_DURATION_MS);

  // =========================================================================
  // SCENE 2: Solid Green (10 seconds)
  // =========================================================================
#ifdef ENABLE_SERIAL
  Serial.println(F("Scene 2: Solid Green (10s)..."));
#endif
  setGreenBrightness(MAX_BRIGHTNESS);
  delay(LIGHT_SCENE_DURATION_MS);

  // =========================================================================
  // SCENE 3: Breathe (10 seconds)
  // =========================================================================
#ifdef ENABLE_SERIAL
  Serial.println(F("Scene 3: Breathing (10s)..."));
#endif
  runBreatheScene(LIGHT_SCENE_DURATION_MS);

  // =========================================================================
  // SCENE 4: Off / Dark (15 seconds)
  // =========================================================================
#ifdef ENABLE_SERIAL
  Serial.println(F("Scene 4: Dark (15s)..."));
#endif
  setGreenBrightness(0);
  delay(OFF_SCENE_DURATION_MS);
}