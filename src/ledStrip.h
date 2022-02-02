#ifndef __LED_STRING_H
#define __LED_STRING_H

#include <FastLED.h>

#define LED_PIN     13
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    28
#define CENTER_LED  13

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60
extern int ledAnimationDelay;
extern CRGB ledStrip[NUM_LEDS];
enum LedAnimation{e_allOff, e_cyclon, e_cyclon2, e_cyclonLeft, e_cyclonRight, e_oneGlow, e_oneGlowRSSIDelay, e_glowing, e_SpinningSinWave, e_pride, e_allOn, e_questionClock, e_twinkle, e_confetti, e_rainbow, e_rainbowGlitter, e_allOnGlitter, e_ledAnimations_max};
extern int animationStartDelay;
extern int animationDelay[e_ledAnimations_max];
extern LedAnimation ledAnimation;
extern CRGB ledAnimationColor;

void pride();
void confetti();
void rainbowWithGlitter();
void addGlitter( fract8 chanceOfGlitter);
void rainbow();
void greenLedStrip();
void redLedStrip();
void CenterToLeft(byte red, byte green, byte blue);
void CenterToRight(byte red, byte green, byte blue);
void SpinningSinWave(CRGB color, int darkSpotCount);
void setPixels(CRGB color, int from, int to);
void setAll(CRGB color);
void glowing(CRGB color);
void oneGlow(CRGB color, int count, int duration);
void cyclon(int start, int end, CRGB color);
void cyclonMiddle(int start, int end, CRGB color);
void questionClock(CRGB color, int count, int duration);



// Overall twinkle speed.
// 0 (VERY slow) to 8 (VERY fast).  
// 4, 5, and 6 are recommended, default is 4.
#define TWINKLE_SPEED 4

// Overall twinkle density.
// 0 (NONE lit) to 8 (ALL lit at once).  
// Default is 5.
#define TWINKLE_DENSITY 5

// How often to change color palettes.
#define SECONDS_PER_PALETTE  30
// Also: toward the bottom of the file is an array 
// called "ActivePaletteList" which controls which color
// palettes are used; you can add or remove color palettes
// from there freely.

// Background color for 'unlit' pixels
// Can be set to CRGB::Black if desired.
extern CRGB gBackgroundColor;
// Example of dim incandescent fairy light background color
// CRGB gBackgroundColor = CRGB(CRGB::FairyLight).nscale8_video(16);

// If AUTO_SELECT_BACKGROUND_COLOR is set to 1,
// then for any palette where the first two entries 
// are the same, a dimmed version of that color will
// automatically be used as the background color.
#define AUTO_SELECT_BACKGROUND_COLOR 0

// If COOL_LIKE_INCANDESCENT is set to 1, colors will 
// fade out slighted 'reddened', similar to how
// incandescent bulbs change color as they get dim down.
#define COOL_LIKE_INCANDESCENT 1
extern CRGBPalette16 gCurrentPalette;
extern CRGBPalette16 gTargetPalette;
void drawTwinkles();
CRGB computeOneTwinkle( uint32_t ms, uint8_t salt);
uint8_t attackDecayWave8( uint8_t i);
void coolLikeIncandescent( CRGB& c, uint8_t phase);
void chooseNextColorPalette( CRGBPalette16& pal);

#endif