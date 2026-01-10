/*
  ANAVI Word Clock - Clock Functions Implementation
  Functions for display, time handling, and LED effects
*/

#include "clock.h"
#include "config.h"
#include <Arduino.h>

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask()
{

  for (byte i = 0; i < 64; i++)
  {
    boolean masker = bitRead(mask, 63 - i); // bitread is backwards because bitRead reads rightmost digits first. could have defined the word masks differently
    switch (masker)
    {
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
uint32_t Wheel(byte WheelPos)
{

  WheelPos = 255 - WheelPos;
  uint32_t wheelColor;

  if (WheelPos < 85)
  {
    wheelColor = matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    wheelColor = matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
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
void rainbowCycle(uint8_t wait)
{
  
  uint16_t i, j;

  // Cycly through all colors
  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < matrix.numPixels(); i++)
    {
      matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
    }
    matrix.show();
    delay(wait);
  }
}

void adjustBrightness()
{
  
  //change brightness if it's night time
  if (theTime.hour() < MORNINGCUTOFF || theTime.hour() > NIGHTCUTOFF)
  {
    matrix.setBrightness(NIGHTBRIGHTNESS);
  }
  else
  {
    matrix.setBrightness(DAYBRIGHTNESS);
  }
}

// function to generate the right "phrase" based on the time

void displayTime(void)
{
  
  // time we display the appropriate theTime.minute() counter
  if ((theTime.minute() > 4) && (theTime.minute() < 10))
  {
    MFIVE;
  }
  if ((theTime.minute() > 9) && (theTime.minute() < 15))
  {
    MTEN;
  }
  if ((theTime.minute() > 14) && (theTime.minute() < 20))
  {
    AQUARTER;
  }
  if ((theTime.minute() > 19) && (theTime.minute() < 25))
  {
    TWENTY;
  }
  if ((theTime.minute() > 24) && (theTime.minute() < 30))
  {
    TWENTY;
    MFIVE;
  }
  if ((theTime.minute() > 29) && (theTime.minute() < 35))
  {
    HALF;
  }
  if ((theTime.minute() > 34) && (theTime.minute() < 40))
  {
    TWENTY;
    MFIVE;
  }
  if ((theTime.minute() > 39) && (theTime.minute() < 45))
  {
    TWENTY;
  }
  if ((theTime.minute() > 44) && (theTime.minute() < 50))
  {
    AQUARTER;
  }
  if ((theTime.minute() > 49) && (theTime.minute() < 55))
  {
    MTEN;
  }
  if (theTime.minute() > 54)
  {
    MFIVE;
  }

  if ((theTime.minute() < 5))
  {
    switch (theTime.hour())
    {
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
    switch (theTime.hour())
    {
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
    switch (theTime.hour())
    {
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

void flashWords(void)
{

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
