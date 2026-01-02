/*
  ANAVI Word Clock
  8x8 WS2812B LEDs using XIAO ESP32C3

  grid pattern

  A T W E N T Y D
  Q U A R T E R Y
  F I V E H A L F
  D P A S T O R O
  F I V E I G H T
  S I X T H R E E
  T W E L E V E N
  F O U R N I N E


  Acknowledgements:
  - Thanks to Andy Doro and Dano for the inspiration:
  https://andydoro.com/wordclockdesktop/

*/

// include the library code:
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "RTClib.h"

// WiFi credentials
const char* ssid = "YOUR-WIFI-NET";
const char* password = "YOUR-WIFI-PASS";

// NTP server setup
WiFiUDP ntpUDP;
// UTC+2 for Bulgaria
NTPClient timeClient(ntpUDP, "pool.ntp.org", 2 * 3600); 

// define how to write each of the words

// 64-bit "mask" for each pixel in the matrix- is it on or off?
uint64_t mask;

// define masks for each word. we add them with "bitwise or" to generate a mask for the entire "phrase".
#define MFIVE    mask |= 0xF0000000000 // these are in hexadecimal, 00 for an empty row
#define MTEN     mask |= 0x1A00000000000000
#define AQUARTER mask |= 0x1FE000000000000
#define TWENTY   mask |= 0x7E00000000000000
#define HALF     mask |= 0xF00000000000
#define PAST     mask |= 0x7800000000
#define TO       mask |= 0xC00000000
#define ONE      mask |= 0x43
#define TWO      mask |= 0x340
#define THREE    mask |= 0x1F0000
#define FOUR     mask |= 0xF0
#define FIVE     mask |= 0xF000000
#define SIX      mask |= 0xE00000
#define SEVEN    mask |= 0x80F000
#define EIGHT    mask |= 0xF8000000
#define NINE     mask |= 0xF
#define TEN      mask |= 0x80018000
#define ELEVEN   mask |= 0xFC00
#define TWELVE   mask |= 0x6F00
#define ANAVI    mask |= 0x1100200000002004
#define WIFI     mask |= 0x400020003000000
#define HA       mask |= 0x300000000000

// define pins
#define NEOPIN 10  // connect to DIN on NeoMatrix 8x8

// brightness based on time of day- could try warmer colors at night?
#define DAYBRIGHTNESS 40
#define NIGHTBRIGHTNESS 20

// cutoff times for day / night brightness. feel free to modify.
#define MORNINGCUTOFF 7  // when does daybrightness begin?   7am
#define NIGHTCUTOFF   22 // when does nightbrightness begin? 10pm


// define delays
#define FLASHDELAY 100  // delay for startup "flashWords" sequence
#define SHIFTDELAY 100   // controls color shifting speed

// Define US or EU rules for DST comment out as required. More countries could be added with different rules in DST_RTC.cpp
const char rulesDST[] = "US"; // US DST rules
// const char rulesDST[] = "EU";   // EU DST rules

DateTime theTime; // Holds current clock time

// an integer for the color shifting effect
int j;

// configure for 8x8 neopixel matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, NEOPIN,
                            NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB         + NEO_KHZ800);

// change brightness based on the time of day.

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask() {

  for (byte i = 0; i < 64; i++) {
    boolean masker = bitRead(mask, 63 - i); // bitread is backwards because bitRead reads rightmost digits first. could have defined the word masks differently
    switch (masker) {
      case 0:
        matrix.setPixelColor(i, 0, 0, 0);
        break;
      case 1:
        matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
        break;
    }
  }

  matrix.show(); // show it!
  delay(SHIFTDELAY);
  j++; // move the colors forward
  j = j % (256 * 5);

  // reset mask for next time
  mask = 0;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {

  WheelPos = 255 - WheelPos;
  uint32_t wheelColor;

  if (WheelPos < 85) {
    wheelColor = matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    wheelColor = matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    wheelColor = matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  // convert from 24-bit to 16-bit color - NeoMatrix requires 16-bit. perhaps there's a better way to do this.
  uint32_t bits = wheelColor;
  uint32_t blue = bits & 0x001F;     // 5 bits blue
  uint32_t green = bits & 0x07E0;    // 6 bits green
  uint32_t red = bits & 0xF800;      // 5 bits red

  // Return shifted bits with alpha set to 0xFF
  return (red << 8) | (green << 5) | (blue << 3) | 0xFF000000;
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  
  uint16_t i, j;

  // Cycly through all colors
  for (j = 0; j < 256; j++) {
    for (i = 0; i < matrix.numPixels(); i++) {
      matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
    }
    matrix.show();
    delay(wait);
  }
}

void adjustBrightness() {
  
  //change brightness if it's night time
  if (theTime.hour() < MORNINGCUTOFF || theTime.hour() > NIGHTCUTOFF) {
    matrix.setBrightness(NIGHTBRIGHTNESS);
  } else {
    matrix.setBrightness(DAYBRIGHTNESS);
  }
}

// function to generate the right "phrase" based on the time

void displayTime(void) {
  
  // time we display the appropriate theTime.minute() counter
  if ((theTime.minute() > 4) && (theTime.minute() < 10)) {
    MFIVE;
    Serial.print("five");
  }
  if ((theTime.minute() > 9) && (theTime.minute() < 15)) {
    MTEN;
    Serial.print("ten");
  }
  if ((theTime.minute() > 14) && (theTime.minute() < 20)) {
    AQUARTER;
    Serial.print("a quarter");
  }
  if ((theTime.minute() > 19) && (theTime.minute() < 25)) {
    TWENTY;
    Serial.print("twenty");
  }
  if ((theTime.minute() > 24) && (theTime.minute() < 30)) {
    TWENTY;
    MFIVE;
    Serial.print("twenty five");
  }
  if ((theTime.minute() > 29) && (theTime.minute() < 35)) {
    HALF;
    Serial.print("half");
  }
  if ((theTime.minute() > 34) && (theTime.minute() < 40)) {
    TWENTY;
    MFIVE;
    Serial.print("twenty five");
  }
  if ((theTime.minute() > 39) && (theTime.minute() < 45)) {
    TWENTY;
    Serial.print("twenty");
  }
  if ((theTime.minute() > 44) && (theTime.minute() < 50)) {
    AQUARTER;
    Serial.print("a quarter");
  }
  if ((theTime.minute() > 49) && (theTime.minute() < 55)) {
    MTEN;
    Serial.print("ten");
  }
  if (theTime.minute() > 54) {
    MFIVE;
    Serial.print("five");
  }

  if ((theTime.minute() < 5))
  {
    switch (theTime.hour()) {
      case 1:
      case 13:
        ONE;
        break;
      case 2:
      case 14:
        TWO;
        break;
      case 3:
      case 15:
        THREE;
        break;
      case 4:
      case 16:
        FOUR;
        break;
      case 5:
      case 17:
        FIVE;
        break;
      case 6:
      case 18:
        SIX;
        break;
      case 7:
      case 19:
        SEVEN;
        break;
      case 8:
      case 20:
        EIGHT;
        break;
      case 9:
      case 21:
        NINE;
        break;
      case 10:
      case 22:
        TEN;
        break;
      case 11:
      case 23:
        ELEVEN;
        break;
      case 0:
      case 12:
        TWELVE;
        break;
    }

  }
  else if ((theTime.minute() < 35) && (theTime.minute() > 4))
  {
    PAST;
    Serial.print(" past ");
    switch (theTime.hour()) {
      case 1:
      case 13:
        ONE;
        break;
      case 2:
      case 14:
        TWO;
        break;
      case 3:
      case 15:
        THREE;
        break;
      case 4:
      case 16:
        FOUR;
        break;
      case 5:
      case 17:
        FIVE;
        break;
      case 6:
      case 18:
        SIX;
        break;
      case 7:
      case 19:
        SEVEN;
        break;
      case 8:
      case 20:
        EIGHT;
        break;
      case 9:
      case 21:
        NINE;
        break;
      case 10:
      case 22:
        TEN;
        break;
      case 11:
      case 23:
        ELEVEN;
        break;
      case 0:
      case 12:
        TWELVE;
        break;
    }
  }
  else
  {
    // if we are greater than 34 minutes past the hour then display
    // the next hour, as we will be displaying a 'to' sign
    TO;
    Serial.print(" to ");
    switch (theTime.hour()) {
      case 1:
      case 13:
        TWO;
        break;
      case 14:
      case 2:
        THREE;
        break;
      case 15:
      case 3:
        FOUR;
        break;
      case 4:
      case 16:
        FIVE;
        break;
      case 5:
      case 17:
        SIX;
        break;
      case 6:
      case 18:
        SEVEN;
        break;
      case 7:
      case 19:
        EIGHT;
        break;
      case 8:
      case 20:
        NINE;
        break;
      case 9:
      case 21:
        TEN;
        break;
      case 10:
      case 22:
        ELEVEN;
        break;
      case 11:
      case 23:
        TWELVE;
        break;
      case 0:
      case 12:
        ONE;
        break;
    }
  }

  // Apply phrase mask to colorshift function
  applyMask();

}

void flashWords(void) {

  ANAVI;
  applyMask();
  delay(FLASHDELAY * 2);

  MFIVE;
  applyMask();
  delay(FLASHDELAY);
  
  MTEN;
  applyMask();
  delay(FLASHDELAY);

  AQUARTER;
  applyMask();
  delay(FLASHDELAY);

  TWENTY;
  applyMask();
  delay(FLASHDELAY);

  HALF;
  applyMask();
  delay(FLASHDELAY);

  TO;
  applyMask();
  delay(FLASHDELAY);

  PAST;
  applyMask();
  delay(FLASHDELAY);
  

  ONE;
  applyMask();
  delay(FLASHDELAY);

  TWO;
  applyMask();
  delay(FLASHDELAY);
  
  THREE;
  applyMask();
  delay(FLASHDELAY);
  
  FOUR;
  applyMask();
  delay(FLASHDELAY);

  FIVE;
  applyMask();
  delay(FLASHDELAY);

  SIX;
  applyMask();
  delay(FLASHDELAY);

  SEVEN;
  applyMask();
  delay(FLASHDELAY);

  EIGHT;
  applyMask();
  delay(FLASHDELAY);


  NINE;
  applyMask();
  delay(FLASHDELAY);

  TEN;
  applyMask();
  delay(FLASHDELAY);

  ELEVEN;
  applyMask();
  delay(FLASHDELAY);

  TWELVE;
  applyMask();
  delay(FLASHDELAY);

  // blank for a bit
  applyMask();
  delay(FLASHDELAY);

}

void setup() {
  // put your setup code here, to run once:

  // set pinmodes
  pinMode(NEOPIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();

  matrix.begin();
  matrix.setBrightness(DAYBRIGHTNESS);
  // Initialize all pixels to 'off'
  matrix.fillScreen(0); 
  matrix.show();

  // Startup
  rainbowCycle(5);
  delay(100);
  flashWords();
  delay(100);

  WIFI;
  applyMask();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  timeClient.begin();
  timeClient.update();

  HA;
  applyMask();
  delay(3000);

}

void loop() {

  timeClient.update();
  
  unsigned long epochTime = timeClient.getEpochTime();
  theTime = DateTime(epochTime);

  Serial.print("BG Time: ");
  Serial.print(theTime.year(), DEC);
  Serial.print('/');
  Serial.print(theTime.month(), DEC);
  Serial.print('/');
  Serial.print(theTime.day(), DEC);
  Serial.print(" ");
  Serial.print(theTime.hour(), DEC);
  Serial.print(':');
  Serial.print(theTime.minute(), DEC);
  Serial.print(':');
  Serial.println(theTime.second(), DEC);

  adjustBrightness();
  displayTime();
  delay(5);
}


