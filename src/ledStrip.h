#ifndef __LED_STRING_H
#define __LED_STRING_H

#include <FastLED.h>

#define LED_PIN     13
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    28
#define CENTER_LED  14

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60
extern CRGB ledStrip[NUM_LEDS];
enum LedAnimation{e_allOff, e_cyclon, e_cyclon2, e_glowing, e_SpinningSinWave, e_pride, e_allOn, e_questionClock, e_ledAnimations_max};
extern LedAnimation ledAnimation;
extern CRGB ledAnimationColor;

void pride();
void greenLedStrip();
void redLedStrip();
void CenterToLeft(byte red, byte green, byte blue);
void CenterToRight(byte red, byte green, byte blue);
void SpinningSinWave(CRGB color, int darkSpotCount);
void setPixels(CRGB color, int from, int to);
void setAll(CRGB color);
void glowing(CRGB color, int speed);
void cyclon(int start, int end);
void cyclonMiddle(int start, int end);
void questionClock(CRGB color, int count, int duration);
#endif