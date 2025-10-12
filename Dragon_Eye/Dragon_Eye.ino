/*
Dragon eyes - Fade up light fadedown wait.
*/
#include <Arduino.h>
#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 14
#define LED_STRIP 4
#define FULL

int iFadeAmount = 15;
int iBrightness = 255;

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
  Serial.begin(115200);
  Serial.println("BLINK setup starting");
  FastLED.addLeds<WS2812B, LED_STRIP, GRB>(leds, NUM_LEDS); 
  FastLED.setBrightness(iBrightness);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  pinMode(LED_STRIP, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  iBrightness = iBrightness + iFadeAmount;
  if (iBrightness < 0) iBrightness = 0;
  if (iBrightness > 255) iBrightness = 255;
  if (iBrightness == 0 || iBrightness == 255) {
    iFadeAmount = -iFadeAmount;
  }
  Serial.print("Dragon eye - Brighness : ");
  Serial.println(iBrightness);
  FastLED.setBrightness(iBrightness);
  fill_solid(leds, NUM_LEDS, CRGB::Orange);
  FastLED.show();
  if (iBrightness == 255) 
    delay(2000);
  else if (iBrightness == 0) 
    delay(5000);
  else
    delay(100);                      
}