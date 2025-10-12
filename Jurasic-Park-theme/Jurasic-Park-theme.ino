/*
*
*
*
*
*/


//Define Pins.
#define PIN_FlashLight 4

void setup() {
  // put your setup code here, to run once:
  pinMode( PIN_FlashLight, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(PIN_FlashLight, HIGH);  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(PIN_FlashLight, LOW);   // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);     
}
