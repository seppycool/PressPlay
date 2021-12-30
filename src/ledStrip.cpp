#include "ledStrip.h"
#include "stdint.h"
#include <FastLED.h>

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.

CRGB ledStrip[NUM_LEDS];
LedAnimation ledAnimation = e_allOff;
CRGB ledAnimationColor;

void pride() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( ledStrip[pixelnumber], newcolor, 64);
  }
}


// Set all LEDs to a given color and apply it (visible)
void setPixel(int index, byte red, byte green, byte blue) {
    ledStrip[index].r = red;
    ledStrip[index].g = green;
    ledStrip[index].b = blue;
}

void setPixel(int index, CRGB color) {
    ledStrip[index].r = color.r;
    ledStrip[index].g = color.g;
    ledStrip[index].b = color.b;
}

// Set all LEDs to a given color and apply it (visible)
void setAll(CRGB color) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, color);
  }
  FastLED.show();
}
void setPixels(CRGB color, int from, int to){
  for(int i = from; i<to; i++){
    setPixel(i,color);
  }
  FastLED.show();
}

void glowing(CRGB color, int speed){
  static double glowIndex = 0;
  glowIndex +=(speed*2*PI/360);
  CRGB newColor;
  newColor.r = ((sin(glowIndex)*127+128)/255)*color.r;
  newColor.g = ((sin(glowIndex)*127+128)/255)*color.g;
  newColor.b = ((sin(glowIndex)*127+128)/255)*color.b;
  setAll(newColor);
}

void SpinningSinWave(CRGB color, int darkSpotCount){
  static int index = 14;
  static double conversion = (darkSpotCount*2*PI/NUM_LEDS);
  index++;
  
  setAll(CRGB(0,0,0));  
  for(int i = 0 ; i<NUM_LEDS; i++){
    setPixel(i,((sin((i+index)*conversion) * 127 + 128)/255)*color.r,
                   ((sin((i+index)*conversion) * 127 + 128)/255)*color.g,
                   ((sin((i+index)*conversion) * 127 + 128)/255)*color.b);
  }
  FastLED.show();
}

void CenterToRight(byte red, byte green, byte blue){
  static int index = CENTER_LED;
  (index++)%NUM_LEDS; 
  
  setAll(CRGB(0,0,0)); 
  for(int i = 0 ; i<NUM_LEDS/2; i++){
    setPixel(i, cos((i+index)*(PI/(NUM_LEDS)))*red,     
                cos((i+index)*(PI/(NUM_LEDS)))*green,
                cos((i+index)*(PI/(NUM_LEDS)))*blue);
  }
  FastLED.show();
}


void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { ledStrip[i].nscale8(200); } }

void cyclon(int start, int end) { 
  static uint8_t hue = 0;
  static int8_t index = start;
  static bool direction = true;
  // First slide the led in one direction
  // Set the i'th led to red 
  ledStrip[index] = CHSV(hue++, 255, 255);
  // Show the leds
  FastLED.show(); 
  // now that we've shown the leds, reset the i'th led to black
  // leds[i] = CRGB::Black;
  fadeall();
  // Wait a little bit before we loop around and do it again
  vTaskDelay(pdMS_TO_TICKS(100));
  
  if(direction){
    index++;
    if(index>end){
      direction = !direction;
      index--;
    }
  }
  else{
    index--;
    if(index<start){
      direction = !direction;
      index++;
    }
  }
}

void cyclon2(int start, int end) { 
  static uint8_t hue = 0;
  int middle = (start + end)/2;
  static int8_t index = 0;
  static bool direction = true;
  // First slide the led in one direction
  // Set the i'th led to red 
  ledStrip[middle + index] = CHSV(hue++, 255, 255);
  ledStrip[middle - index] = CHSV(hue++, 255, 255);
  // Show the leds
  FastLED.show(); 
  // now that we've shown the leds, reset the i'th led to black
  // leds[i] = CRGB::Black;
  fadeall();
  // Wait a little bit before we loop around and do it again
  vTaskDelay(pdMS_TO_TICKS(100));
  
  if(direction){
    index++;
    if((middle+index)>end){
      direction = !direction;
      index--;
    }
  }
  else{
    index--;
    if((index)<0){
      direction = !direction;
      index++;
    }
  }
}



