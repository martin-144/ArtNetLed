#ifndef torch_h
#define torch_h

#include <FastLED.h>
#include <inttypes.h>
#include <stdint.h>

// ArtNet Settings, defined in artnet.h
extern uint8_t artnetLevels;
extern uint8_t artnetLevelsRaw;
extern uint16_t dmxChannel;

// Number of LEDs around the tube. One too much looks better (italic text look)
// than one to few (backwards leaning text look)
// Higher number = diameter of the torch gets larger
// Ast 8
// Lamp 14
const uint8_t ledsPerLevel = 14;

// Number of "windings" of the LED strip around (or within) the tube
// Higher number = torch gets taller
// Ast 26
// Lamp 28
const uint8_t levels = 28;
uint8_t artnetLevels;

// Dim the flame when going below this number of levels
const uint8_t dimmingLevel = 4;

// set to true if you wound the torch clockwise (as seen from top). Note that
// this reverses the entire animation (in contrast to mirrorText, which only
// mirrors text).
const bool reversedX = true;

// set to true if you wound the torch from top down
const bool reversedY = true;

// set to true if every other row in the LED matrix is ordered backwards.
// This mode is useful for WS2812 modules which have e.g. 16x16 LEDs on one
// flexible PCB. On these modules, the data line starts in the lower left
// corner, goes right for row 0, then left in row 1, right in row 2 etc.
const bool alternatingX = false;

// set to true if your WS2812 chain runs up (or up/down, with alternatingX set) the "torch",
// for example if you want to do a wide display out of multiple 16x16 arrays
const bool swapXY = false;

// const bool xReversed = false; // even (0,2,4...) rows go backwards, or all if not alternating
// const bool yReversed = false; // Y reversed
// const bool alternating = false; // direction changes after every row

const uint16_t numLeds = ledsPerLevel * levels; // total number of LEDs

CRGB leds[numLeds];

// global parameters
uint8_t currentEnergy[numLeds]; // current energy level
uint8_t nextEnergy[numLeds]; // next energy level
uint8_t energyMode[numLeds]; // mode how energy is calculated for this point

enum {
    torch_passive = 0, // just environment, glow from nearby radiation
    torch_nop = 1, // no processing
    torch_spark = 2, // slowly looses energy, moves up
    torch_spark_temp = 3, // a spark still getting energy from the level below
};

enum { // moving lamp mode params
    mode_off = 0,
    mode_torch = 1, // torch
    mode_colorcycle = 2,
    mode_lamp = 3 // lamp
};

// torch parameters

uint8_t flame_min = 20; // 0..255
uint8_t flame_max = 220; // 0..255

uint8_t random_spark_probability = 5; // 0..100
uint8_t spark_min = 200; // 0..255
uint8_t spark_max = 255; // 0..255

uint8_t spark_tfr = 40; // 0..256 how much energy is transferred up for a spark per cycle
uint8_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint8_t up_rad = 35; // up radiation
uint8_t side_rad = 35; // sidewards radiationc//
uint8_t heat_cap = 0; // 0..255: passive cells: how much energy is retained from previous cycle

uint8_t red_bg = 0;
uint8_t green_bg = 0;
uint8_t blue_bg = 0;
uint8_t red_bias = 55;
uint8_t green_bias = 0;
uint8_t blue_bias = 0;
uint8_t red_energy = 255;
uint8_t green_energy = 100;
uint8_t blue_energy = 0;

uint8_t brightness = 255; // overall brightness
uint8_t fade_base = 140; // crossfading base brightness level

// void torch(struct led *ws2812_framebuffer);
uint8_t torch_random(uint8_t aMinOrMax, uint8_t aMax);
void calcNextEnergy();
void calcNextColors();
void injectRandom(); // lamp mode params
void reduce(uint8_t *aByte, uint8_t aAmount, uint8_t aMin);
void increase(uint8_t *aByte, uint8_t aAmount, uint8_t aMax);
void setColorXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue);
void setColor(struct CRGB* leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue);

void setColorDimmedXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue, byte aBrightness);
void setColorDimmed(struct CRGB* leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness);

void calcNextEnergy()
{
    int i = 0;
    for (int y=0; y<levels; y++) {
        for (int x=0; x<ledsPerLevel; x++) {
            uint8_t e = currentEnergy[i];
            uint8_t m = energyMode[i];
            switch (m) {
                case torch_spark: {
                    // loose transfer up energy as long as the is any
                    reduce(&e, spark_tfr, 0);
                    // cell above is temp spark, sucking up energy from this cell until empty
                    if (y < artnetLevels-1) {
                        energyMode[i+ledsPerLevel] = torch_spark_temp;
                    }
                    // 20191206 by Martin; If artnetlevels < ledsPerLevel set LEDs above off
                    else if (y > artnetLevels-1) {
                        energyMode[i+ledsPerLevel] = torch_passive;
                    }
                    break;
                }
                case torch_spark_temp: {
                    // just getting some energy from below
                    uint8_t e2 = currentEnergy[i-ledsPerLevel];
                    if (e2<spark_tfr) {
                        // cell below is exhausted, becomes passive
                        energyMode[i-ledsPerLevel] = torch_passive;
                        // gobble up rest of energy
                        increase(&e, e2, 255);
                        // loose some overall energy
                        e = ((int)e*spark_cap)>>8;
                        // this cell becomes active spark
                        energyMode[i] = torch_spark;
                    }
                    else {
                        increase(&e, spark_tfr, 255);
                    }
                    break;
                }
                case torch_passive: {
                    e = ((int)e*heat_cap)>>8;
                    increase(&e, ((((int)currentEnergy[i-1]+(int)currentEnergy[i+1])*side_rad)>>9) + (((int)currentEnergy[i-ledsPerLevel]*up_rad)>>8), 255);
                }
                default:
                break;
            }
            nextEnergy[i++] = e;
        }
    }
}
const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};

void calcNextColors(CRGB color)
{
    for (int i=0; i<numLeds; i++) {
        uint16_t e = nextEnergy[i];
        currentEnergy[i] = e;

        if (e>250)
        setColorDimmed(leds, i, 170, 170, e, brightness);
        else {
            if (e>0) {
                // energy to brightness is non-linear
                uint8_t eb = energymap[e>>3];
                uint8_t r = red_bias;
                uint8_t g = green_bias;
                uint8_t b = blue_bias;
                increase(&r, (eb*color.r)>>8, 255);
                increase(&g, (eb*color.g)>>8, 255);
                increase(&b, (eb*color.b)>>8, 255);
                setColorDimmed(leds, i, r, g, b, brightness);
            }
            else {
                // background, no energy
                setColorDimmed(leds, i, red_bg, green_bg, blue_bg, brightness);
            }
        }
    }
}


void injectRandom()
{
    // random flame energy at bottom row
    for (int i=0; i<ledsPerLevel; i++) {
        currentEnergy[i] = torch_random(flame_min, flame_max);
        energyMode[i] = torch_nop;
    }
    // random sparks at second row
    for (int i=ledsPerLevel; i<2*ledsPerLevel; i++)
    {
        if (energyMode[i]!=torch_spark && torch_random(100, 0)<random_spark_probability)
        {
            currentEnergy[i] = torch_random(spark_min, spark_max);
            energyMode[i] = torch_spark;
        }
    }
}

// Utilities
// =========

/* Dim Torch when <= artnet_levels
if (artnetLevels <= dimmingLevel)
{
  // Serial.printf("levels = %d, dimming_level = %d\n", levels, dimming_level);
  // Serial.printf("Dimming Level = %d\n", map(artnet_levels_raw, 0, ledsPerLevel * dimmingLevel, 0, 255));
  // brightness = map(artnetLevelsRaw, 0, 60, 0, brightness);
  brightness = map(artnetTorchParams.param1, 0, 60, 0, artnetTorchParams.brightness); // why is it 60 ?
}
*/

int getNumPixels()
{
    return numLeds;
}


uint16_t getSizeX()
{
    return swapXY ? levels : ledsPerLevel;
}


uint16_t getSizeY()
{
    return swapXY ? ledsPerLevel : levels;
}

void resetEnergy()
{
    for (int i=0; i<numLeds; i++) {
        currentEnergy[i] = 0;
        nextEnergy[i] = 0;
        energyMode[i] = torch_passive;
    }
}

uint8_t torch_random(uint8_t aMinOrMax, uint8_t aMax)
{
    if (aMax==0)
    {
        aMax = aMinOrMax;
        aMinOrMax = 0;
    }
    uint32_t r = aMinOrMax;
    aMax = aMax - aMinOrMax + 1;
    r += rand() % aMax;
    return r;
}


void reduce(uint8_t *aByte, uint8_t aAmount, uint8_t aMin)
{
    int r = *aByte-aAmount;
    if (r<aMin)
    *aByte = aMin;
    else
    *aByte = (uint8_t)r;
}


void increase(uint8_t *aByte, uint8_t aAmount, uint8_t aMax)
{
    int r = *aByte+aAmount;
    if (r>aMax)
    *aByte = aMax;
    else
    *aByte = (uint8_t)r;
}


/*
uint16_t ledIndexFromXY(uint16_t aX, uint16_t aY)
{
if (swapXY) { uint16_t tmp = aY; aY = aX; aX = tmp; }
if (yReversed) { aY = levels-1-aY; }
uint16_t ledindex = aY*ledsPerLevel;
bool reversed = xReversed;
if (alternating) {
if (aY & 0x1) reversed = !reversed;
}
if (reversed) {
ledindex += (ledsPerLevel-1-aX);
}
else {
ledindex += aX;
}
return ledindex;
}

void setColorXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue)
{
uint16_t ledindex = ledIndexFromXY(aX,aY);
if (ledindex>=numPixels) return;
RGBPixel *pixP = &(pixelBufferP[ledindex]);
// linear brightness is stored with 5bit precision only
pixP->red = aRed>>3;
pixP->green = aGreen>>3;
pixP->blue = aBlue>>3;
}
*/

void setColor(struct CRGB *leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue)
{
    if (aLedNumber>=numLeds) return; // invalid LED number
    // linear brightness is stored with 5bit precision only
    leds[aLedNumber].red = aRed;
    leds[aLedNumber].green = aGreen;
    leds[aLedNumber].blue = aBlue;
}

void setColorDimmed(struct CRGB *leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness)
{
    setColor(leds, aLedNumber, (aRed*aBrightness)>>8, (aGreen*aBrightness)>>8, (aBlue*aBrightness)>>8);
}

#endif /* torch_h */
