#include <FastLED.h>  // Main FastLED library for controlling LEDs

#define LED_STRIP 4
#include <Arduino.h>
#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 14

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI


// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
    Serial.begin(115200);
    Serial.println("BLINK setup starting");
    
    FastLED.addLeds<WS2812B, LED_STRIP>(leds, NUM_LEDS);  // GRB ordering is assumed

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  pinMode(LED_STRIP, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  // Turn the LED on, then pause
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(1000);                      // wait for a second
  
  // Now turn the LED off, then pause
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second

}