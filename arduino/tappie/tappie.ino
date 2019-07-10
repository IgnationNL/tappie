#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 10
#define LED_NUM 16
#define RELAY_PIN 6

#define POURING_TIME_THRESHOLD 5000 // Time after which to stop pouring.
#define DETECT_CUP_THRESHOLD 4000 // Time after which to scan for cup

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

String cupID = ""; // Id of the CUP that was last detected.
bool isPouring = false;
bool cupIsPresent = false;
bool cupValidationInProgress = false;
bool pouringIsComplete = false;
long timeSincePouringStart = 0; // Time how long pouring is busy
long timeSinceLastCupCheck = 0;

bool ledTimeTurnOn = true; // Used as an indication to turn on or off the leds

void setup(void) {
    Serial.begin(9600);
    nfc.begin();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
  
    // LED
    strip.begin();
    strip.show();   
}

void loop(void) {

    // Detect if a cup is present.
    if ((millis() - timeSinceLastCupCheck) >= DETECT_CUP_THRESHOLD) {
      Serial.println("checking cup");
      timeSinceLastCupCheck = millis();

      if (nfc.tagPresent()) {
        cupIsPresent = true;
       
        NfcTag tag = nfc.read();

        if (tag.getUidString() != cupID) { // Detected new cup. Start validation
          cupID = tag.getUidString();
          didDetectCup(cupID);
        }
    } else {
      cupIsPresent = false;
    }
    }
    
        
    if (isPouring == true && (millis() - timeSincePouringStart) >= POURING_TIME_THRESHOLD) { // Pouring complete
      stopPouring();
    } else if (isPouring == true) { // Busy pouring
      ledPouringInProgress();
      Serial.println("busy pour");   
    } else if (cupValidationInProgress == true) { // Cup validation in progress
      ledCupValidationInProgress();
    } else if (cupIsPresent && pouringIsComplete) {
       ledShowComplete(ledTimeTurnOn);
    } else if (cupIsPresent && !isPouring && !cupValidationInProgress) { // There is a cup present, but nothing is happening.
      ledError(ledTimeTurnOn);
    } else if (!cupIsPresent) { // There is no cup present
      pouringIsComplete = false;
      cupID = "";

      //digitalWrite(RELAY_PIN, HIGH); // Make sure the relay is turned off.
      
      // Show screensaver
      ledScreensaver();
    }
    
    if (cupValidationInProgress && Serial.available() > 0) { // Cup validation complete
      String incoming = Serial.readString();
      
      didValidateCup(incoming == "1"); // 0 is not validated, 1 is validated.
    }
    
    ledTimeTurnOn = !ledTimeTurnOn;
    delay(500);
}

// A cup was detected by the NFC reader
void didDetectCup(String cupId) {
  cupValidationInProgress = true;
  Serial.println("did detect cup");
  Serial.println(cupId);  // Send cupID for verification to Raspi.
}

// Raspi validated the cup: pouring can start or will be denied.
void didValidateCup(bool isValid) {
  cupValidationInProgress = false;
  
  if (isValid) { // Cup is valid. We can start pouring.
    Serial.println("did validate succ");
    startPouring();
  } else { // Cup is not valided.
    Serial.println("not validated");
  }
}

// Starts the pouring of liquid.
void startPouring() {
  isPouring = true;
  timeSincePouringStart = millis();
  digitalWrite(RELAY_PIN, LOW);

  Serial.println("start pour");
}

// Ends the pouring of liquid.
void stopPouring() {
  isPouring = false;
  pouringIsComplete = true;
  timeSincePouringStart = 0;
  digitalWrite(RELAY_PIN, HIGH);
  ledPouringComplete();


  Serial.println("stop pour");
}

// LED methods

void ledSetColor(short r, short g, short b) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
   strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// Shown when tappie is ready to go
void ledScreensaver() {
  ledSetColor(50,50,50);
}

void ledShowComplete(bool colourIsOn) {
  if (colourIsOn) {
    ledSetColor(0,255,0);
  } else {
    ledSetColor(0,0,0);
  }
}

// Shown when an error occured.
void ledError(bool colourIsOn) {

  if (colourIsOn) {
    ledSetColor(255,0,0);
  } else {
    ledSetColor(0,0,0);
  }
  
}

// Shown when cup validation is in progress
void ledCupValidationInProgress() {
  ledSetColor(0,0,255);
}

// Shown when pouring is in progress.
void ledPouringInProgress() {


  double procentComplete = ((millis() - timeSincePouringStart) / (POURING_TIME_THRESHOLD / 100));

  int ledsToTurnOn = LED_NUM * (procentComplete / 100);

  if (ledsToTurnOn < 2) {
    ledSetColor(0,0,0);
  } else {
    for(uint16_t i=0; i<ledsToTurnOn; i++) {
      Serial.println("turning on led");
   strip.setPixelColor(i, strip.Color(0, 255, 0));
   
  strip.show();
   delay(50);
  }
  }
  
}

// Shown when pouring is complete
void ledPouringComplete() {
  ledSetColor(0,255,0);
}


