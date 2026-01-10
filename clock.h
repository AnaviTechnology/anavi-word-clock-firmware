/*
  ANAVI Word Clock - Clock Functions Header
  WordClock class for display, time handling, and LED effects
*/

#ifndef CLOCK_FUNCTIONS_H
#define CLOCK_FUNCTIONS_H

#include <Adafruit_NeoMatrix.h>
#include <RTClib.h>

class WordClock {
public:
    // Constructor
    WordClock();
    
    // Public methods used in setup() and loop()
    void begin();

    void setBrightness(uint8_t brightness);

    void rainbowCycle(uint8_t wait);

    void flashWords();

    void adjustBrightness(const DateTime& currentTime);

    void displayTime(const DateTime& currentTime);

    void showStatusWiFi();

    void showStatusHomeAssistant();
    
private:
    // Private member variables
    Adafruit_NeoMatrix matrix;
    uint64_t mask;
    int colorShift;
    
    // Brightness settings
    uint8_t dayBrightness;
    uint8_t nightBrightness;
    uint8_t morningCutoff;
    uint8_t nightCutoff;
    
    // Timing delays
    uint16_t flashDelay;
    uint16_t shiftDelay;
    
    // Private methods
    void applyMask();
    uint32_t Wheel(byte wheelPos);
    
    // Word mask setting methods
    void setMFive();
    void setMTen();
    void setAQuarter();
    void setTwenty();
    void setHalf();
    void setPast();
    void setTo();
    void setOne();
    void setTwo();
    void setThree();
    void setFour();
    void setFive();
    void setSix();
    void setSeven();
    void setEight();
    void setNine();
    void setTen();
    void setEleven();
    void setTwelve();
    void setAnavi();
    void setWifi();
    void setHA();
};

#endif // CLOCK_FUNCTIONS_H
