#include "stdint.h"

// Number of LEDs around the tube. One too much looks better (italic text look)
// than one to few (backwards leaning text look)
// Higher number = diameter of the torch gets larger
const uint16_t ledsPerLevel = 8;

// Number of "windings" of the LED strip around (or within) the tube
// Higher number = torch gets taller
const uint16_t levels = 26;

// set to true if you wound the torch clockwise (as seen from top). Note that
// this reverses the entire animation (in contrast to mirrorText, which only
// mirrors text).
const bool reversedX = false;

// set to true if you wound the torch from top down
const bool reversedY = false;

// set to true if every other row in the LED matrix is ordered backwards.
// This mode is useful for WS2812 modules which have e.g. 16x16 LEDs on one
// flexible PCB. On these modules, the data line starts in the lower left
// corner, goes right for row 0, then left in row 1, right in row 2 etc.
const bool alternatingX = false;
// set to true if your WS2812 chain runs up (or up/down, with alternatingX set) the "torch",
// for example if you want to do a wide display out of multiple 16x16 arrays
const bool swapXY = false;

const uint16_t numLeds = ledsPerLevel*levels; // total number of LEDs

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

enum {
  mode_off = 0,
  mode_torch = 1, // torch
  mode_colorcycle = 2, // moving color cycle
  mode_lamp = 3, // lamp
};

// torch parameters

uint16_t cycle_wait = 25; // 0..255

uint8_t flame_min = 20; // 0..255
uint8_t flame_max = 220; // 0..255

uint8_t random_spark_probability = 5; // 0..100
uint8_t spark_min = 200; // 0..255
uint8_t spark_max = 255; // 0..255

uint8_t spark_tfr = 40; // 0..256 how much energy is transferred up for a spark per cycle
uint16_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint16_t up_rad = 35; // up radiation
uint16_t side_rad = 35; // sidewards radiation
uint16_t heat_cap = 0; // 0..255: passive cells: how much energy is retained from previous cycle

uint8_t red_bg = 1;
uint8_t green_bg = 0;
uint8_t blue_bg = 0;
uint8_t red_bias = 20;
uint8_t green_bias = 0;
uint8_t blue_bias = 0;
int red_energy = 180;
int green_energy = 100;
int blue_energy = 0;


// lamp mode params

uint8_t lamp_red = 220;
uint8_t lamp_green = 220;
uint8_t lamp_blue = 200;

uint8_t mode = mode_torch; // main operation mode
int brightness = 255; // overall brightness
uint8_t fade_base = 140; // crossfading base brightness level

// void torch(struct led *ws2812_framebuffer);
void calcNextEnergy();
void calcNextColors();
void injectRandom();
uint8_t torch_random(uint8_t aMinOrMax, uint8_t aMax);
void reduce(uint8_t *aByte, uint8_t aAmount, uint8_t aMin);
void increase(uint8_t *aByte, uint8_t aAmount, uint8_t aMax);
void setColor(struct CRGB* leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue);
void setColorDimmed(struct CRGB* leds, uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness);
