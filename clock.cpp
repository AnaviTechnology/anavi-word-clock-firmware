/*
  ANAVI Word Clock - Clock Functions Implementation
  WordClock class for display, time handling, and LED effects
*/

#include "clock.h"
#include "config.h"
#include <Arduino.h>

// Word mask definitions (64-bit masks for 8x8 matrix)
#define MASK_MFIVE    0xF0000000000ULL
#define MASK_MTEN     0x1A00000000000000ULL
#define MASK_AQUARTER 0x1FE000000000000ULL
#define MASK_TWENTY   0x7E00000000000000ULL
#define MASK_HALF     0xF00000000000ULL
#define MASK_PAST     0x7800000000ULL
#define MASK_TO       0xC00000000ULL
#define MASK_ONE      0x43ULL
#define MASK_TWO      0x340ULL
#define MASK_THREE    0x1F0000ULL
#define MASK_FOUR     0xF0ULL
#define MASK_FIVE     0xF000000ULL
#define MASK_SIX      0xE00000ULL
#define MASK_SEVEN    0x80F000ULL
#define MASK_EIGHT    0xF8000000ULL
#define MASK_NINE     0xFULL
#define MASK_TEN      0x80018000ULL
#define MASK_ELEVEN   0xFC00ULL
#define MASK_TWELVE   0x6F00ULL
#define MASK_ANAVI    0x1100200000002004ULL
#define MASK_WIFI     0x400020003000000ULL
#define MASK_HA       0x300000000000ULL

WordClock::WordClock()
    : matrix(8, 8, NEOPIN,
             NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
             NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
             NEO_GRB         + NEO_KHZ800)
    , mask(0)
    , colorShift(0)
    , dayBrightness(40)
    , nightBrightness(20)
    , morningCutoff(7)
    , nightCutoff(22)
    , flashDelay(100)
    , shiftDelay(100)
{
}

void WordClock::begin()
{
    matrix.begin();
    matrix.setBrightness(dayBrightness);
    matrix.fillScreen(0);
    matrix.show();
}

void WordClock::setBrightness(uint8_t brightness)
{
    matrix.setBrightness(brightness);
}

void WordClock::applyMask()
{
    for (byte i = 0; i < 64; i++)
    {
        // bitread is backwards because bitRead reads rightmost digits first
        boolean masker = bitRead(mask, 63 - i);
        switch (masker)
        {
            case 0:
                matrix.setPixelColor(i, 0, 0, 0);
                break;
            case 1:
                matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + colorShift) & 255));
                break;
        }
    }

    matrix.show();
    delay(shiftDelay);
    colorShift++;
    colorShift = colorShift % (256 * 5);

    // reset mask for next time
    mask = 0;
}

uint32_t WordClock::Wheel(byte wheelPos)
{
    wheelPos = 255 - wheelPos;
    uint32_t wheelColor;

    if (wheelPos < 85)
    {
        wheelColor = matrix.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    else if (wheelPos < 170)
    {
        wheelPos -= 85;
        wheelColor = matrix.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    else
    {
        wheelPos -= 170;
        wheelColor = matrix.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    }

    // convert from 24-bit to 16-bit color
    uint32_t bits = wheelColor;
    uint32_t blue = bits & 0x001F;
    uint32_t green = bits & 0x07E0;
    uint32_t red = bits & 0xF800;

    return (red << 8) | (green << 5) | (blue << 3) | 0xFF000000;
}

void WordClock::rainbowCycle(uint8_t wait)
{
    uint16_t i, j;

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

void WordClock::adjustBrightness(const DateTime& currentTime)
{
    if (currentTime.hour() < morningCutoff || currentTime.hour() > nightCutoff)
    {
        matrix.setBrightness(nightBrightness);
    }
    else
    {
        matrix.setBrightness(dayBrightness);
    }
}

void WordClock::displayTime(const DateTime& currentTime)
{
    // Display the appropriate minute counter
    if ((currentTime.minute() > 4) && (currentTime.minute() < 10))
    {
        setMFive();
    }
    if ((currentTime.minute() > 9) && (currentTime.minute() < 15))
    {
        setMTen();
    }
    if ((currentTime.minute() > 14) && (currentTime.minute() < 20))
    {
        setAQuarter();
    }
    if ((currentTime.minute() > 19) && (currentTime.minute() < 25))
    {
        setTwenty();
    }
    if ((currentTime.minute() > 24) && (currentTime.minute() < 30))
    {
        setTwenty();
        setMFive();
    }
    if ((currentTime.minute() > 29) && (currentTime.minute() < 35))
    {
        setHalf();
    }
    if ((currentTime.minute() > 34) && (currentTime.minute() < 40))
    {
        setTwenty();
        setMFive();
    }
    if ((currentTime.minute() > 39) && (currentTime.minute() < 45))
    {
        setTwenty();
    }
    if ((currentTime.minute() > 44) && (currentTime.minute() < 50))
    {
        setAQuarter();
    }
    if ((currentTime.minute() > 49) && (currentTime.minute() < 55))
    {
        setMTen();
    }
    if (currentTime.minute() > 54)
    {
        setMFive();
    }

    if ((currentTime.minute() < 5))
    {
        switch (currentTime.hour())
        {
            case 1:
            case 13:
                setOne();
                break;
            case 2:
            case 14:
                setTwo();
                break;
            case 3:
            case 15:
                setThree();
                break;
            case 4:
            case 16:
                setFour();
                break;
            case 5:
            case 17:
                setFive();
                break;
            case 6:
            case 18:
                setSix();
                break;
            case 7:
            case 19:
                setSeven();
                break;
            case 8:
            case 20:
                setEight();
                break;
            case 9:
            case 21:
                setNine();
                break;
            case 10:
            case 22:
                setTen();
                break;
            case 11:
            case 23:
                setEleven();
                break;
            case 0:
            case 12:
                setTwelve();
                break;
        }
    }
    else if ((currentTime.minute() < 35) && (currentTime.minute() > 4))
    {
        setPast();
        switch (currentTime.hour())
        {
            case 1:
            case 13:
                setOne();
                break;
            case 2:
            case 14:
                setTwo();
                break;
            case 3:
            case 15:
                setThree();
                break;
            case 4:
            case 16:
                setFour();
                break;
            case 5:
            case 17:
                setFive();
                break;
            case 6:
            case 18:
                setSix();
                break;
            case 7:
            case 19:
                setSeven();
                break;
            case 8:
            case 20:
                setEight();
                break;
            case 9:
            case 21:
                setNine();
                break;
            case 10:
            case 22:
                setTen();
                break;
            case 11:
            case 23:
                setEleven();
                break;
            case 0:
            case 12:
                setTwelve();
                break;
        }
    }
    else
    {
        // if we are greater than 34 minutes past the hour then display
        // the next hour, as we will be displaying a 'to' sign
        setTo();
        switch (currentTime.hour())
        {
            case 1:
            case 13:
                setTwo();
                break;
            case 14:
            case 2:
                setThree();
                break;
            case 15:
            case 3:
                setFour();
                break;
            case 4:
            case 16:
                setFive();
                break;
            case 5:
            case 17:
                setSix();
                break;
            case 6:
            case 18:
                setSeven();
                break;
            case 7:
            case 19:
                setEight();
                break;
            case 8:
            case 20:
                setNine();
                break;
            case 9:
            case 21:
                setTen();
                break;
            case 10:
            case 22:
                setEleven();
                break;
            case 11:
            case 23:
                setTwelve();
                break;
            case 0:
            case 12:
                setOne();
                break;
        }
    }

    // Apply phrase mask to colorshift function
    applyMask();
}

void WordClock::showStatusWiFi()
{
    setWifi();
    applyMask();
} 

void WordClock::showStatusHomeAssistant()
{
    setHA();
    applyMask();
}

void WordClock::flashWords()
{
    setAnavi();
    applyMask();
    delay(flashDelay * 2);

    setMFive();
    applyMask();
    delay(flashDelay);
    
    setMTen();
    applyMask();
    delay(flashDelay);

    setAQuarter();
    applyMask();
    delay(flashDelay);

    setTwenty();
    applyMask();
    delay(flashDelay);

    setHalf();
    applyMask();
    delay(flashDelay);

    setTo();
    applyMask();
    delay(flashDelay);

    setPast();
    applyMask();
    delay(flashDelay);

    setOne();
    applyMask();
    delay(flashDelay);

    setTwo();
    applyMask();
    delay(flashDelay);
    
    setThree();
    applyMask();
    delay(flashDelay);
    
    setFour();
    applyMask();
    delay(flashDelay);

    setFive();
    applyMask();
    delay(flashDelay);

    setSix();
    applyMask();
    delay(flashDelay);

    setSeven();
    applyMask();
    delay(flashDelay);

    setEight();
    applyMask();
    delay(flashDelay);

    setNine();
    applyMask();
    delay(flashDelay);

    setTen();
    applyMask();
    delay(flashDelay);

    setEleven();
    applyMask();
    delay(flashDelay);

    setTwelve();
    applyMask();
    delay(flashDelay);

    // blank for a bit
    applyMask();
    delay(flashDelay);
}

// Word mask setter methods
void WordClock::setMFive()    { mask |= MASK_MFIVE; }
void WordClock::setMTen()     { mask |= MASK_MTEN; }
void WordClock::setAQuarter() { mask |= MASK_AQUARTER; }
void WordClock::setTwenty()   { mask |= MASK_TWENTY; }
void WordClock::setHalf()     { mask |= MASK_HALF; }
void WordClock::setPast()     { mask |= MASK_PAST; }
void WordClock::setTo()       { mask |= MASK_TO; }
void WordClock::setOne()      { mask |= MASK_ONE; }
void WordClock::setTwo()      { mask |= MASK_TWO; }
void WordClock::setThree()    { mask |= MASK_THREE; }
void WordClock::setFour()     { mask |= MASK_FOUR; }
void WordClock::setFive()     { mask |= MASK_FIVE; }
void WordClock::setSix()      { mask |= MASK_SIX; }
void WordClock::setSeven()    { mask |= MASK_SEVEN; }
void WordClock::setEight()    { mask |= MASK_EIGHT; }
void WordClock::setNine()     { mask |= MASK_NINE; }
void WordClock::setTen()      { mask |= MASK_TEN; }
void WordClock::setEleven()   { mask |= MASK_ELEVEN; }
void WordClock::setTwelve()   { mask |= MASK_TWELVE; }
void WordClock::setAnavi()    { mask |= MASK_ANAVI; }
void WordClock::setWifi()     { mask |= MASK_WIFI; }
void WordClock::setHA()       { mask |= MASK_HA; }
