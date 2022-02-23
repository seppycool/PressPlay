#include "ledStrip.h"
#include "stdint.h"
#include <FastLED.h>

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.

CRGB ledStrip[NUM_LEDS];
LedAnimation ledAnimation = e_allOff;
int animationDelay[e_ledAnimations_max] = {100,50,100,50,50,100,100,10,100,100,100,100,100, 100, 50, 50, 100};
int animationStartDelay = 0;
int ledAnimationDelay = 1000/FRAMES_PER_SECOND;
CRGB ledAnimationColor = CRGB::Red;

void pride() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (45 * 256), (60 * 256)); // change color speed --> higher is slower
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

void rainbow() 
{
  static uint8_t gHue = 0;
  // FastLED's built-in rainbow generator
  fill_rainbow( ledStrip, NUM_LEDS, gHue++, 7);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    ledStrip[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void confetti() 
{
  static uint8_t gHue = 0;
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( ledStrip, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  ledStrip[pos] += CHSV( (gHue++) + random8(64), 200, 255);
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
}
void setPixels(CRGB color, int from, int to){
  for(int i = from; i<to; i++){
    setPixel(i,color);
  }
}

void glowing(CRGB color){
  static double glowIndex = 0;
  glowIndex +=2*PI/360;
  CRGB newColor;
  newColor.r = ((sin(glowIndex)*127+128)/255)*color.r;
  newColor.g = ((sin(glowIndex)*127+128)/255)*color.g;
  newColor.b = ((sin(glowIndex)*127+128)/255)*color.b;
  setAll(newColor);
  FastLED.show();
}

void oneGlow(CRGB color, int count, int duration){
  setAll(CRGB(0,0,0));
  double glowIndex = (double)count/(double)duration;
  CRGB newColor;
  newColor.r = glowIndex*color.r;
  newColor.g = glowIndex*color.g;
  newColor.b = glowIndex*color.b;
  setAll(newColor);
  FastLED.show();
}

void questionClock(CRGB color, int count, int duration){
  setAll(CRGB(0,0,0));
  int i = 0;
  while(i<((NUM_LEDS - (NUM_LEDS*(double)count/(double)duration)))){
    setPixel(i,color);
    i++;
  }
  double fade = NUM_LEDS*(double)count/(double)duration - (int)(NUM_LEDS*(double)count/(double)duration);
  setPixel(i,color.fadeLightBy(fade*255));
  FastLED.show();
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

void cyclon(int start, int end, CRGB color) { 
  static uint8_t hue = 0;
  static int8_t index = start;
  static bool direction = true;
  // First slide the led in one direction
  // Set the i'th led to red 
  ledStrip[index] = color;// CHSV(hue++, 255, 255);
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

void cyclonMiddle(int start, int end, CRGB color) { 
  static uint8_t hue = 0;
  int middle = (start + end)/2;
  static int8_t index = 0;
  static bool direction = true;
  // First slide the led in one direction
  // Set the i'th led to red 
  setPixel(middle + index, color);
  setPixel(middle - index, color);
  //ledStrip[middle + index] = CHSV(hue++, 255, 255);
  //ledStrip[middle - index] = CHSV(hue++, 255, 255);
  // Show the leds
  FastLED.show(); 
  // now that we've shown the leds, reset the i'th led to black
  // leds[i] = CRGB::Black;
  fadeall();
  
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


CRGB gBackgroundColor = CRGB::Black; 
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;
//  This function loops over each pixel, calculates the 
//  adjusted 'clock' that this pixel should use, and calls 
//  "CalculateOneTwinkle" on each pixel.  It then displays
//  either the twinkle color of the background color, 
//  whichever is brighter.
static int counter = 0;
void drawTwinkles()
{
  if(!(counter++% 200)) chooseNextColorPalette( gTargetPalette );
  nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 12);

  // "PRNG16" is the pseudorandom number generator
  // It MUST be reset to the same starting value each time
  // this function is called, so that the sequence of 'random'
  // numbers that it generates is (paradoxically) stable.
  uint16_t PRNG16 = 11337;
  
  uint32_t clock32 = millis();

  // Set up the background color, "bg".
  // if AUTO_SELECT_BACKGROUND_COLOR == 1, and the first two colors of
  // the current palette are identical, then a deeply faded version of
  // that color is used for the background color
  CRGB bg;
  if( (AUTO_SELECT_BACKGROUND_COLOR == 1) &&
      (gCurrentPalette[0] == gCurrentPalette[1] )) {
    bg = gCurrentPalette[0];
    uint8_t bglight = bg.getAverageLight();
    if( bglight > 64) {
      bg.nscale8_video( 16); // very bright, so scale to 1/16th
    } else if( bglight > 16) {
      bg.nscale8_video( 64); // not that bright, so scale to 1/4th
    } else {
      bg.nscale8_video( 86); // dim, scale to 1/3rd.
    }
  } else {
    bg = gBackgroundColor; // just use the explicitly defined background color
  }

  uint8_t backgroundBrightness = bg.getAverageLight();
  
  for(int i = 0; i<NUM_LEDS; i++) {
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    uint16_t myclockoffset16= PRNG16; // use that number as clock offset
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    // use that number as clock speed adjustment factor (in 8ths, from 8/8ths to 23/8ths)
    uint8_t myspeedmultiplierQ5_3 =  ((((PRNG16 & 0xFF)>>4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
    uint32_t myclock30 = (uint32_t)((clock32 * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
    uint8_t  myunique8 = PRNG16 >> 8; // get 'salt' value for this pixel

    // We now have the adjusted 'clock' for this pixel, now we call
    // the function that computes what color the pixel should be based
    // on the "brightness = f( time )" idea.
    CRGB c = computeOneTwinkle( myclock30, myunique8);

    uint8_t cbright = c.getAverageLight();
    int16_t deltabright = cbright - backgroundBrightness;
    if( deltabright >= 32 || (!bg)) {
      // If the new pixel is significantly brighter than the background color, 
      // use the new color.
      setPixel(i,c);
    } else if( deltabright > 0 ) {
      // If the new pixel is just slightly brighter than the background color,
      // mix a blend of the new color and the background color
      setPixel(i,blend( bg, c, deltabright * 8));
    } else { 
      // if the new pixel is not at all brighter than the background color,
      // just use the background color.
      setPixel(i,bg);
    }
  }
}


//  This function takes a time in pseudo-milliseconds,
//  figures out brightness = f( time ), and also hue = f( time )
//  The 'low digits' of the millisecond time are used as 
//  input to the brightness wave function.  
//  The 'high digits' are used to select a color, so that the color
//  does not change over the course of the fade-in, fade-out
//  of one cycle of the brightness wave function.
//  The 'high digits' are also used to determine whether this pixel
//  should light at all during this cycle, based on the TWINKLE_DENSITY.
CRGB computeOneTwinkle( uint32_t ms, uint8_t salt)
{
  uint16_t ticks = ms >> (8-TWINKLE_SPEED);
  uint8_t fastcycle8 = ticks;
  uint16_t slowcycle16 = (ticks >> 8) + salt;
  slowcycle16 += sin8( slowcycle16);
  slowcycle16 =  (slowcycle16 * 2053) + 1384;
  uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);
  
  uint8_t bright = 0;
  if( ((slowcycle8 & 0x0E)/2) < TWINKLE_DENSITY) {
    bright = attackDecayWave8( fastcycle8);
  }

  uint8_t hue = slowcycle8 - salt;
  CRGB c;
  if( bright > 0) {
    c = ColorFromPalette( gCurrentPalette, hue, bright, NOBLEND);
    if( COOL_LIKE_INCANDESCENT == 1 ) {
      coolLikeIncandescent( c, fastcycle8);
    }
  } else {
    c = CRGB::Black;
  }
  return c;
}


// This function is like 'triwave8', which produces a 
// symmetrical up-and-down triangle sawtooth waveform, except that this
// function produces a triangle wave with a faster attack and a slower decay:
//
//     / \ 
//    /     \ 
//   /         \ 
//  /             \ 
//

uint8_t attackDecayWave8( uint8_t i)
{
  if( i < 86) {
    return i * 3;
  } else {
    i -= 86;
    return 255 - (i + (i/2));
  }
}

// This function takes a pixel, and if its in the 'fading down'
// part of the cycle, it adjusts the color a little bit like the 
// way that incandescent bulbs fade toward 'red' as they dim.
void coolLikeIncandescent( CRGB& c, uint8_t phase)
{
  if( phase < 128) return;

  uint8_t cooling = (phase - 128) >> 4;
  c.g = qsub8( c.g, cooling);
  c.b = qsub8( c.b, cooling * 2);
}

// A mostly red palette with green accents and white trim.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedGreenWhite_p FL_PROGMEM =
{  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
   CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
   CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray, 
   CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green };

// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const TProgmemRGBPalette16 Holly_p FL_PROGMEM =
{  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
   Holly_Green, Holly_Green, Holly_Green, Holly_Red 
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedWhite_p FL_PROGMEM =
{  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
   CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
   CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
   CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A mostly blue palette with white accents.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 BlueWhite_p FL_PROGMEM =
{  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
   CRGB::Blue, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A pure "fairy light" palette with some brightness variations
#define HALFFAIRY ((CRGB::FairyLight & 0xFEFEFE) / 2)
#define QUARTERFAIRY ((CRGB::FairyLight & 0xFCFCFC) / 4)
const TProgmemRGBPalette16 FairyLight_p FL_PROGMEM =
{  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, 
   HALFFAIRY,        HALFFAIRY,        CRGB::FairyLight, CRGB::FairyLight, 
   QUARTERFAIRY,     QUARTERFAIRY,     CRGB::FairyLight, CRGB::FairyLight, 
   CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight };

// A palette of soft snowflakes with the occasional bright one
const TProgmemRGBPalette16 Snow_p FL_PROGMEM =
{  0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0x304048,
   0x304048, 0x304048, 0x304048, 0xE0F0FF };

// A palette reminiscent of large 'old-school' C9-size tree lights
// in the five classic colors: red, orange, green, blue, and white.
#define C9_Red    0xB80400
#define C9_Orange 0x902C02
#define C9_Green  0x046002
#define C9_Blue   0x070758
#define C9_White  0x606820
const TProgmemRGBPalette16 RetroC9_p FL_PROGMEM =
{  C9_Red,    C9_Orange, C9_Red,    C9_Orange,
   C9_Orange, C9_Red,    C9_Orange, C9_Red,
   C9_Green,  C9_Green,  C9_Green,  C9_Green,
   C9_Blue,   C9_Blue,   C9_Blue,
   C9_White
};

// A cold, icy pale blue palette
#define Ice_Blue1 0x0C1040
#define Ice_Blue2 0x182080
#define Ice_Blue3 0x5080C0
const TProgmemRGBPalette16 Ice_p FL_PROGMEM =
{
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue2, Ice_Blue2, Ice_Blue2, Ice_Blue3
};


// Add or remove palette names from this list to control which color
// palettes are used, and in what order.
const TProgmemRGBPalette16* ActivePaletteList[] = {
  &RetroC9_p,
  &BlueWhite_p,
  &RainbowColors_p,
  &FairyLight_p,
  &RedGreenWhite_p,
  &PartyColors_p,
  &RedWhite_p,
  &Snow_p,
  &Holly_p,
  &Ice_p  
};

// Advance to the next color palette in the list (above).
void chooseNextColorPalette( CRGBPalette16& pal)
{
  const uint8_t numberOfPalettes = sizeof(ActivePaletteList) / sizeof(ActivePaletteList[0]);
  static uint8_t whichPalette = -1; 
  whichPalette = addmod8( whichPalette, 1, numberOfPalettes);

  pal = *(ActivePaletteList[whichPalette]);
}//= sine( time ); // a sine wave of brightness over time