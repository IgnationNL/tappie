#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 10
#define LED_NUM 16
#define RELAY_PIN 6

#define POURING_TIME_THRESHOLD 5000 // Time after which to stop pouring.
#define CLEANING_TIME_THRESHOLD 8000 // Time after which to stop pouring.
#define DETECT_CUP_THRESHOLD 4000 // Time after which to scan for cup

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

String cupID = ""; // Id of the CUP that was last detected.
bool raspiIsConnected = false;
bool isPouring = false;
bool isCleaning = false;
bool cupIsPresent = false;
bool cupValidationInProgress = false;
bool pouringIsComplete = false;
bool cleaningIsComplete = false;
long timeSincePouringStart = 0; // Time how long pouring is busy
long timeSinceCleaningStart = 0; // Time how long cleaning is busy
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
    if ((millis() - timeSinceLastCupCheck) >= DETECT_CUP_THRESHOLD && raspiIsConnected) {
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
    
    if (isCleaning == true && (millis() - timeSinceCleaningStart) >= CLEANING_TIME_THRESHOLD) {
      stopCleaning();
    } else if (isPouring == true && (millis() - timeSincePouringStart) >= POURING_TIME_THRESHOLD) { // Pouring complete
      stopPouring();
    } else if (isPouring == true || isCleaning == true) { // Busy pouring or cleaning
      ledPouringInProgress(isCleaning);
      Serial.println("busy pour/ clean");   
    } else if (cupValidationInProgress == true) { // Cup validation in progress
      ledCupValidationInProgress();
    } else if (cupIsPresent && (pouringIsComplete || cleaningIsComplete)) {
       ledShowComplete(ledTimeTurnOn);
    } else if (cupIsPresent && !isPouring && !cupValidationInProgress && !isCleaning) { // There is a cup present, but nothing is happening.
      ledError(ledTimeTurnOn);
    } else if (!cupIsPresent) { // There is no cup present. Reset values.
      pouringIsComplete = false;
      cleaningIsComplete = false;
      cupID = "";
 
      // Show screensaver
      ledScreensaver();
    }
    
    if ((cupValidationInProgress || !raspiIsConnected) && Serial.available() > 0) { // Received input from raspi
      String incoming = Serial.readString();

      if (incoming == "R") {
        Serial.println("RASPI CONNECTED");
        raspiIsConnected = true;
      } else {
        didValidateCup(incoming == "1"); // 0 is not validated, 1 is validated.
      }
    }
    
    ledTimeTurnOn = !ledTimeTurnOn;
    delay(500);
}

// A cup was detected by the NFC reader
void didDetectCup(String cupId) {

  if (cupID == "04 C6 AE 02 74 4C 80") { // Cleaning cup
    if (!isCleaning) { // Start cleaning if not already busy.
      startCleaning();
    }
  } else if (cupID == "09 09 09") { // Valid example cup
    didValidateCup(true);
  } else if (cupID == "01 01 01") { // Invalid example cup
    didValidateCup(false);
  } else {
    cupValidationInProgress = true;
    Serial.println("did detect cup");
    Serial.println(cupId);  // Send cupID for verification to Raspi.
  }
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

void startCleaning() {
  isCleaning = true;
  timeSinceCleaningStart = millis();
  digitalWrite(RELAY_PIN, LOW);

  Serial.println("start clean");
}

void stopCleaning() {
  isCleaning = false;
  cleaningIsComplete = true;
  timeSinceCleaningStart = 0;
  digitalWrite(RELAY_PIN, HIGH);
  ledShowComplete(ledTimeTurnOn);


  Serial.println("cleaning completed");
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
  ledShowComplete(ledTimeTurnOn);

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
  if (raspiIsConnected) {
    ledSetColor(50,50,50);
  } else {
    ledSetColor(255,0,0);
  }
}

void ledShowComplete(bool colourIsOn) {
  if (colourIsOn) {
    for (uint16_t i=0; i<80;i++) {
      short colour = 30 + (2 * i);
      ledSetColor(0,colour,0);
      delay(10);
    }
  } else {
    for (uint16_t i=0; i<80;i++) {
          short colour = 190 - (2 * i);
          ledSetColor(0,colour,0);
          delay(10);
        }
      }
}

// Shown when an error occured.
void ledError(bool colourIsOn) {

  if (colourIsOn) {
    for (uint16_t i=0; i<80;i++) {
      short colour = 30 + (2 * i);
      ledSetColor(colour,0,0);
      delay(10);
    }
  } else {
    for (uint16_t i=0; i<80;i++) {
          short colour = 190 - (2 * i);
          ledSetColor(colour,0,0);
          delay(10);
        }
      }
}

// Shown when cup validation is in progress
void ledCupValidationInProgress() {
  ledSetColor(0,0,100);
}

// Shown when pouring is in progress.
void ledPouringInProgress(bool isCleaning) {
  double procentComplete = ((millis() - timeSincePouringStart) / (POURING_TIME_THRESHOLD / 100));
  
  if (isCleaning) { // Cleaning
    double procentComplete = ((millis() - timeSinceCleaningStart) / (CLEANING_TIME_THRESHOLD / 100));

  } else { 
    // default
  }
  
  int colourBrightness = 255 * (procentComplete / 100);

  ledSetColor(0,colourBrightness,0);
}

