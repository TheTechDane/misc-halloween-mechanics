/*
*
*
*
*
*/
#include "DFRobotDFPlayerMini.h"
#include "ESP32Servo.h"
#include "HardwareSerial.h"

Servo serDinoHead;  // create Servo object to control a servo

//Define Pins.
#define PIN_FlashLight 4
#define PIN_DinoHead 2

// DFPlayer (using Serial Port 2)
#define DFPLAYER_RX_PIN 20 // ESP32 RX2 -> DFPlayer TX
#define DFPLAYER_TX_PIN 21 // ESP32 TX2 -> DFPlayer RX

// --- 3. CREATE OBJECTS ---
Servo myServo;
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial mySerial(1);

int iTrack =0;

/**
 * @brief Checks for and prints messages from the DFPlayer (errors, etc.)
 * This should be called in the main loop()
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
          case Busy:
            Serial.println("Busy");
            break;
          case Sleeping:
            Serial.println("Sleeping");
            break;
          case SerialWrongStack:
            Serial.println("Serial Wrong Stack");
            break;
          case CheckSumNotMatch:
            Serial.println("Check Sum Not Match");
            break;
          case FileIndexOut:
            Serial.println("File Index Out");
            break;
          case FileMismatch:
            Serial.println("File Mismatch");
            break;
          case Advertise:
            Serial.println("Advertise");
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Jurassic Park Rig..");

  // put your setup code here, to run once:
  pinMode(PIN_FlashLight, OUTPUT);
    Serial.println("Jurassic Park Rig.. 1");
  pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("Jurassic Park Rig.. 2");
  serDinoHead.attach(PIN_DinoHead);  // attaches the servo on pin 9 to the Servo object

  // --- Initialize DFPlayer ---
  Serial.println("Initializing DFPlayer...");
  mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);

  if (!myDFPlayer.begin(mySerial, false, false)) {
    Serial.println("!!! Unable to begin DFPlayer. Check wiring. !!!");
    while (true); // Halt
  }
  Serial.println("DFPlayer Mini online.");
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  
  myDFPlayer.volume(15); // Set volume (0-30)
  //myDFPlayer.play(5);    // Play track 5 briefly to "wake up"
  //delay(500);            // This small delay in setup is OK
  //myDFPlayer.stop();
}

void loop() {
  checkDFPlayer(); 

  // put your main code here, to run repeatedly:
  digitalWrite(PIN_FlashLight, HIGH);  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  serDinoHead.write(0);

  Serial.print("Jurassic Park Rig.. ON .. sound");
  Serial.println(iTrack);
  myDFPlayer.play(iTrack);  

  delay(1000);                      // wait for a second
  digitalWrite(PIN_FlashLight, LOW);   // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  serDinoHead.write(90);
  Serial.println("Jurassic Park Rig..OFF");
  delay(1000);    


  //myDFPlayer.stop(); 

  iTrack++;
  if (iTrack > 5) iTrack = 0;
}
