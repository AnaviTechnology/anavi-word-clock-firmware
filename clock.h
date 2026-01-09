/*
  ANAVI Word Clock - Clock Functions Header
  Functions for display, time handling, and LED effects
*/

#ifndef CLOCK_FUNCTIONS_H
#define CLOCK_FUNCTIONS_H

#include <Adafruit_NeoMatrix.h>
#include <RTClib.h>

// External references to global variables defined in main .ino
extern Adafruit_NeoMatrix matrix;
extern uint64_t mask;
extern int j;
extern DateTime theTime;

// Function declarations

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask();

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos);

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait);

void adjustBrightness();

// function to generate the right "phrase" based on the time
void displayTime(void);

void flashWords(void);

#endif // CLOCK_FUNCTIONS_H
