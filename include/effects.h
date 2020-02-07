#ifndef EFFECTS_H
#define EFFECTS_H

#include <FastLED.h>
#include <torch.h>

#define qsubd(x, b)  ((x>b)?b:0)                   // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)                 // Analog Unsigned subtraction macro. if result <0, then => 0

CRGBPalette16 currentPalette;                      // Palette definitions
CRGBPalette16 targetPalette;
TBlendType currentBlending = LINEARBLEND;


void setPixel(int Pixel, CRGB colorRGB)
{
   // FastLED
   leds[Pixel] = colorRGB;
}

void setAll(CRGB color)
{
  fill_solid( leds, numLeds, color);
}

void meteorRain(CRGB colorRGB, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay)
{
  static uint8_t i;

    // fade brightness all LEDs one step
    for(int j=0; j < numLeds; j++)
    {
      if((!meteorRandomDecay) || (random(10)>5))
      {
        leds[j].fadeToBlackBy(meteorTrailDecay);
      }
    }

    // draw meteor
    for(int j = 0; j < meteorSize; j++)
    {
      if((i-j < numLeds) && (i-j>=0))
      {
        // setPixel(i-j, red, green, blue);
        leds[i-j] = colorRGB;
      }
    }
if(i++ == numLeds) {i = 0;}
}

void meteorRainRows(CRGB colorRGB, uint8_t fadeRows)
{
    fadeRows = fadeRows / levels;

    // setAll(0);

    static uint8_t row;

    fadeToBlackBy(leds, numLeds, 128);

    for(uint8_t n = 0; n <= ledsPerLevel; n++)
    {
      if(fadeRows >= row) fadeRows = row;
      for(uint8_t fr = 0; fr <= fadeRows; fr++)
      {
        leds[(row + fr) * ledsPerLevel + n] = colorRGB;

        if(random(15)>5) // && (fr <= row))
        {
          leds[(row + fr) * ledsPerLevel + n].fadeToBlackBy(250);
        }
      }
    }
    if(row++ == levels) {row = 0;}
}

// It's like meteorRainRows, but without the trails
void sparkleUp(CRGB colorRGB, uint8_t fadeRows)
{
    fadeRows = fadeRows / levels;

    setAll(0);

    static uint8_t row;

    for(uint8_t n = 0; n <= ledsPerLevel; n++)
    {
      if(fadeRows >= row) fadeRows = row;
      for(uint8_t fr = 0; fr <= fadeRows; fr++)
      {
        leds[(row + fr) * ledsPerLevel + n] = colorRGB;

        if(random(15)>5) // && (fr <= row))
        {
          leds[(row + fr) * ledsPerLevel + n].fadeToBlackBy(240);
        }
      }
    }
    if(row++ == levels) {row = 0;}
}


void sparkle(CRGB colorRGB)
{
  static uint8_t pixelOn = 1;
  static uint8_t pixel;

  if (pixelOn)
  {
    pixel = random(numLeds);
    setPixel(pixel, colorRGB);
    pixelOn = 0;
  }
  else
  {
    setPixel(pixel, 0);
    pixelOn = 1;
  }
}

void confetti(CRGB colorRGB, uint8_t hueOffset)
{
  CHSV colorHSV = rgb2hsv_approximate(colorRGB);

  // This many pixels will be set per cycle. Gives the effect of speed-up.
  uint8_t numPixel = 3;

  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, numLeds, 10 * numPixel);

  for(uint8_t num = 0; num <= numPixel; num++)
  {
    int pos = random16(numLeds);
    leds[pos] += CHSV(colorHSV.hue - (hueOffset / 2) + random8(hueOffset), 255, 255);
  }
}

void staticRainbow(uint8_t hueOffset)
{
  // built-in static FastLED rainbow
  fill_rainbow(leds, numLeds, 0, hueOffset / 12 + 1);
}

void plasma(uint8_t palette)
{                                                // This is the heart of this program. Sure is short. . . and fast.
  currentPalette = RainbowColors_p;

  int thisPhase = beatsin8(7,-64,64);            // Setting phase change for a couple of waves.
  int thatPhase = beatsin8(13,-255,64);

  for (int i=0; i<numLeds; i++)  // For each of the LED's in the strand, set a brightness based on a wave as follows:
  {
    int colorIndex = cubicwave8((i*23)+thisPhase)/2 + cos8((i*15)+thatPhase)/2;           // Create a wave and add a phase change and add another wave with its own phase change.. Hey, you can even change the frequencies if you wish.
    int thisBright = qsuba(colorIndex, beatsin8(5,0,96));                                 // qsub gives it a bit of 'black' dead space by setting sets a minimum value. If colorIndex < current value of beatsin8(), then bright = 0. Otherwise, bright = colorIndex..

    leds[i] = ColorFromPalette(currentPalette, colorIndex, thisBright, currentBlending);  // Let's now add the foreground colour.
  }
}

void juggle(CRGB colorRGB, uint8_t hueOffset)
{
  CHSV colorHSV = rgb2hsv_approximate(colorRGB);

  static uint8_t    numdots =   1; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    secondHand = 0;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, numLeds, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, numLeds)] += CHSV(colorHSV.hue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

#endif // EFFECTS_H
