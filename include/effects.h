#ifndef EFFECTS_H
#define EFFECTS_H

#include <FastLED.h>
#include <torch.h>

#define qsubd(x, b)  ((x>b)?b:0)                   // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)                 // Analog Unsigned subtraction macro. if result <0, then => 0

CRGBPalette16 currentPalette;                      // Palette definitions
CRGBPalette16 targetPalette;
TBlendType currentBlending = LINEARBLEND;


void setPixel(int Pixel, CRGB color)
{
   // FastLED
   leds[Pixel] = color;
}

void setAll(CRGB color)
{
  for(int i = 0; i < numLeds; i++ )
  {
    setPixel(i, color);
  }
}

void meteorRain(CRGB color, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay)
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
      if(( i-j < numLeds) && (i-j>=0))
      {
        // setPixel(i-j, red, green, blue);
        leds[i-j] = color;
      }
    }
if(i++ == numLeds) {i = 0;}
}

void setStaticColor(CRGB color)
{
    fill_solid( leds, numLeds, color);
}

void meteorRainRows(CRGB color, uint8_t fadeRows)
{
    fadeRows = fadeRows / levels + 1;
    static uint8_t row;

    for(uint8_t n = 0; n <= ledsPerLevel; n++)
    {
      leds[row * ledsPerLevel + n] = color;

      for(uint8_t fr = 0; fr <= fadeRows; fr++)
      {
        if(random(15)>5)
        {
          leds[(row - fr) * ledsPerLevel + n].fadeToBlackBy(255 / fadeRows + 128);
        }
      }
      leds[(row - fadeRows) * ledsPerLevel + n] = 0;
    }
    if(row++ == levels) {row = 0;}
}


void sparkle(CRGB color)
{
  int Pixel = random(numLeds);
  setPixel(Pixel, color);
  FastLED.show();
  EVERY_N_MILLISECONDS(20)
    {
      setPixel(Pixel, CRGB(0,0,0));
    }
}

void plasma()
{                                                // This is the heart of this program. Sure is short. . . and fast.

  int thisPhase = beatsin8(3,-64,64);            // Setting phase change for a couple of waves.
  int thatPhase = beatsin8(13,-255,64);

  for (int k=0; k<numLeds; k++)
  {                              // For each of the LED's in the strand, set a brightness based on a wave as follows:

    int colorIndex = cubicwave8((k*23)+thisPhase)/2 + cos8((k*15)+thatPhase)/2;           // Create a wave and add a phase change and add another wave with its own phase change.. Hey, you can even change the frequencies if you wish.
    int thisBright = qsuba(colorIndex, beatsin8(7,0,96));                                 // qsub gives it a bit of 'black' dead space by setting sets a minimum value. If colorIndex < current value of beatsin8(), then bright = 0. Otherwise, bright = colorIndex..

    leds[k] = ColorFromPalette(currentPalette, colorIndex, thisBright, currentBlending);  // Let's now add the foreground colour.
  }
}

#endif // EFFECTS_H
