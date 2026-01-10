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

// Include configuration header with all constants
#include "config.h"

// Include function headers
#include "clock.h"
#include "network.h"

// include the library code:
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>

DateTime theTime; // Holds current clock time

// Create WordClock instance
WordClock wordClock;

// Create NetworkConnector instance
NetworkConnector networkConnector;

void setup()
{
    // Set pinmodes
    pinMode(NEOPIN, OUTPUT);
    pinMode(pinAlarm, OUTPUT);
    pinMode(pinButton, INPUT);
    pinMode(pinExtra, OUTPUT);
    digitalWrite(pinExtra, LOW);

    Serial.begin(115200);
    Serial.println();

    // Initialize the word clock
    wordClock.begin();

    // Startup animations
    wordClock.rainbowCycle(5);
    delay(100);
    wordClock.flashWords();
    delay(100);

    // Initialize network manager (includes factory reset check, config loading, machine ID)
    networkConnector.begin();

    // Show WiFi status
    wordClock.showStatusWiFi();

    // Setup WiFi connection
    networkConnector.setupWiFi();

    // Show Home Assistant status
    wordClock.showStatusHomeAssistant();

    // Setup MQTT
    networkConnector.setupMQTT();

    // Print configuration summary
    networkConnector.printConfiguration();
}

void loop()
{
    networkConnector.updateTime();
  
    unsigned long epochTime = networkConnector.getEpochTime();
    theTime = DateTime(epochTime);

    wordClock.adjustBrightness(theTime);
    wordClock.displayTime(theTime);
    delay(5);
}
