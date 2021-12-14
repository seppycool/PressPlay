// Copyright (c) 2021 Van Hecke Seppe

//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following
//  conditions:

//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.

//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.

// /* 
// *  version 1.0
// *  @author Seppe Van Hecke
// *  @contact seppevanhecke@gmail.com
// * 
// *  @description
// *   PressPlay Controller
//     a controller is connected via Wifi/Mqtt to a broker
//     the controller has 2 buttons with leds and a screen
// */

#include <string>
#include <iostream>

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <EasyButton.h>

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"
#include "config.h"

const char* ssid       = SSID_NERDLAB;
const char* password   = PASSWORD_NERDLAB;

const char* ssid1       = SSID_PRIVATE;
const char* password1   = PASSWORD_PRIVATE;
const char* mqtt_server = MQTT_SERVER;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define BAT_CURVE_COUNT 14
const int batteryCurve[2][BAT_CURVE_COUNT] = { {5000, 4200,  4060, 3980, 3920, 3870, 3820, 3790, 3770, 3740, 3680, 3450, 3000, 0},
                                               {100 , 100 ,    90,   80,   70,   60,   50,   40,   30,   20,   10,    5,    0, 0}};

float VBAT;  // battery voltage from ESP32 ADC read
int SOC;  //battery SOC percentage
int SOCAlert = 10;

String macAddress = WiFi.macAddress();
const int freq = 5000;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int resolution = 8;

int lightshow = 0;
bool buttonsActive = true;
#define BUTTON1_PIN 14
#define BUTTON1_LED 12
#define BUTTON2_PIN 27
#define BUTTON2_LED 25
#define BATTERY_PIN 35
int button1LedState = LOW;
int button2LedState = LOW;
enum Button{none,button1,button2};
Button lastButtonClicked = none;



struct tm timeinfo;

// Use the corresponding display class:

// Initialize the OLED display using SPI
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi        display(D0, D2, D8);
// or
// SH1106Spi         display(D0, D2);

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo  display(0x3c, D3, D5);



void WiFiMqtt_task(void *pvParameter);
void buttons_task(void *pvParameter);
void screen_task(void *pvParameter);
void battery_timer_callBack( TimerHandle_t xTimer );
void getLocalTime();
void setButtonLedState(Button button);
void setButtonLedState();
void setButtonLedState(Button button, bool state);

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(BATTERY_PIN, INPUT);

  
  xTaskCreatePinnedToCore(&WiFiMqtt_task,"WifiMqttTask",4048,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(&buttons_task,"goButtonTask",2048,NULL,5,NULL,1);
  xTaskCreatePinnedToCore(&screen_task,"screenTask",2048,NULL,5,NULL,1);
  TimerHandle_t batteryTimer_handle = xTimerCreate("BatteryTimerTask",pdMS_TO_TICKS(60000),pdTRUE,( void * ) 0,battery_timer_callBack);
  if( xTimerStart(batteryTimer_handle, 0 ) != pdPASS )
    Serial.println("Timer not started");
  battery_timer_callBack(batteryTimer_handle);
}


void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// WIFI / MQTT /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT
  if (String(topic) == "controllers/button1Led") {
    int dutycycle = atoi(messageTemp.c_str());
    if(dutycycle > 0 && dutycycle<=255 ){
      ledcWrite(ledChannel1,dutycycle);
      button2LedState = dutycycle;
    }
  }
  if (String(topic) == "controllers/button2Led") {
    int dutycycle = atoi(messageTemp.c_str());
    if(dutycycle > 0 && dutycycle<=255 ){
      ledcWrite(ledChannel2,dutycycle);
      button1LedState = dutycycle;
    }
  }
  if (String(topic) == "controllers/lightshow") {
    lightshow = atoi(messageTemp.c_str());
  }
  if (String(topic) == "controllers/alertSOC") {
    int newAlertSOC = atoi(messageTemp.c_str());
    if(newAlertSOC > 0 && newAlertSOC<=100 )
      SOCAlert = newAlertSOC;
  }
  if (String(topic) == "controllers/status") {
    if(messageTemp == "battery"){
      if(SOC<SOCAlert){
        lightshow = 1;
      }
    }
    if(messageTemp == "lastButton"){
      if(lastButtonClicked == button1) {
        setButtonLedState(button1,HIGH);
        setButtonLedState(button2,LOW);
      }
      if(lastButtonClicked == button2){
        setButtonLedState(button1,LOW);
        setButtonLedState(button2,HIGH);
      }
    }
    if(messageTemp == "resetLed"){
      setButtonLedState(button1,LOW);
      setButtonLedState(button2,LOW);
    }
    if(messageTemp == "buttonsDeactivate"){
      buttonsActive = false;
    }
    if(messageTemp == "buttonsActivate"){
      buttonsActive = true;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(macAddress.c_str())) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("controllers/button1Led");
      client.subscribe("controllers/button2Led");
      client.subscribe("controllers/lightshow");
      client.subscribe("controllers/status");
      client.subscribe("controllers/alertSOC");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

void WiFiMqtt_task(void *pvParameter){
    //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  int connectionCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(500));
      Serial.print(".");
      if(connectionCount++==10){

        Serial.printf("Connecting to %s ", ssid1);
        WiFi.begin(ssid1, password1);
      }

  }
  Serial.println(" CONNECTED");
  Serial.println(WiFi.localIP());
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();

  //client.setServer(mqtt_server_nerdlab,1883);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  for(;;){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  } 
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// BUTTONS /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void setButtonLedState(Button button){
  switch (button)
  {
  case button1:
    if(button1LedState) ledcWrite(ledChannel1, 255);
    else ledcWrite(ledChannel1, 0);
    break;
  case button2:
    if(button2LedState) ledcWrite(ledChannel2, 255);
    else ledcWrite(ledChannel2, 0);
    break;
  
  default:
    break;
  }
}
void setButtonLedState(){
  setButtonLedState(button1);
  setButtonLedState(button2);
}

void setButtonLedState(Button button, bool state){
  switch (button)
  {
  case button1:
    button1LedState = state;
    break;
  case button2:
    button2LedState = state;
    break;
  
  default:
    break;
  }
  setButtonLedState(button);
}


void startLightShow1(){
  unsigned startTime = millis();
  while((millis()-startTime)<5000){
    ledcWrite(ledChannel1,random(255));
    ledcWrite(ledChannel2,random(255));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  setButtonLedState();
}
void startLightShow2(){
  unsigned startTime = millis();
  int dutycycle = 0;
  int increment = 1;
  while((millis()-startTime)<20000){
    if(increment>0 && dutycycle>=255){
      increment = -1;
    }
    if(increment<0 && dutycycle<=0){
      increment = 1;
    }
    dutycycle += increment;
    ledcWrite(ledChannel1,dutycycle);
    ledcWrite(ledChannel2,255-dutycycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  setButtonLedState();
}

void onPressedButton1()
{ 
  Serial.println("Button1 pressed");
  getLocalTime();
  lastButtonClicked = button1;

  button1LedState = !button1LedState;
  setButtonLedState(button1);

  String topic = macAddress + "/button1/state";
  String payload = "pressed";
  client.publish(topic.c_str(),payload.c_str());

  topic = "controllers/"+ macAddress + "/input";
  payload = "controllers,id="+macAddress +" button=1";
  client.publish(topic.c_str(),payload.c_str());
  
  topic = macAddress + "/button1/lastPressed";
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  client.publish(topic.c_str(),timeStringBuff);
}
void onPressedButton2()
{ 
  Serial.println("Button2 pressed");
  getLocalTime();
  lastButtonClicked = button2;

  button2LedState = !button2LedState;
  setButtonLedState(button2);

  String topic = macAddress + "/button2/state";
  String payload = "pressed";
  client.publish(topic.c_str(),payload.c_str());

  topic = "controllers/"+ macAddress + "/input";
  payload = "controllers,id="+macAddress +" button=2";

  topic = macAddress + "/button2/lastPressed";
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  client.publish(topic.c_str(),timeStringBuff);
}

void buttons_task(void *pvParameter){
  printf("button1Task is started");
  EasyButton button1(BUTTON1_PIN);
  EasyButton button2(BUTTON2_PIN);

  // configure LED PWM functionalitites
  ledcSetup(ledChannel1, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(BUTTON1_LED, ledChannel1);
  ledcWrite(ledChannel1,0);
  button1LedState = 0;

  // configure LED PWM functionalitites
  ledcSetup(ledChannel2, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(BUTTON2_LED, ledChannel2);
  ledcWrite(ledChannel2,0);
  button2LedState = 0;

  button1.begin();
  button2.begin();
  button1.onPressed(onPressedButton1);
  button2.onPressed(onPressedButton2);

  for(;;){
    if(buttonsActive){
      button1.read();
      button2.read();
    }
    if(lightshow){
      switch (lightshow)
      {
      case 1:
        startLightShow1();
        break;
      case 2:
        startLightShow2();
        break;
      
      default:
        break;
      }
      lightshow = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  } 
}


//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// SCREEN /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the OLED display using Wire library
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui ( &display );
void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void batteryFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { analogClockFrame, digitalClockFrame, batteryFrame};

// how many frames are there?
int frameCount = 3;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;

void getLocalTime()
{

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// utility function for digital clock display: prints leading 0
String twoDigits(int digits) {
  if (digits < 10) {
    String i = '0' + String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {

}

void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  getLocalTime();
  //  ui.disableIndicator();

  // Draw the clock face
  //  display->drawCircle(clockCenterX + x, clockCenterY + y, clockRadius);
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
  //
  //hour ticks
  for ( int z = 0; z < 360; z = z + 30 ) {
    //Begin at 0° and stop at 360°
    float angle = z ;
    angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
    int x2 = ( clockCenterX + ( sin(angle) * clockRadius ) );
    int y2 = ( clockCenterY - ( cos(angle) * clockRadius ) );
    int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
  }

  // display second hand
  float angle = timeinfo.tm_sec * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display minute hand
  angle = timeinfo.tm_min * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display hour hand
  angle = timeinfo.tm_hour * 30 + int( ( timeinfo.tm_min / 12 ) * 6 )   ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
}

void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  getLocalTime();
  String timenow = String(timeinfo.tm_hour) + ":" + twoDigits(timeinfo.tm_min) + ":" + twoDigits(timeinfo.tm_sec);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y, timenow );
}

void batteryFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String vBat = String(VBAT) + " V";
  String socBat = String(SOC)+ " %";
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y, vBat );
  display->drawString(clockCenterX + x , 16 + y, socBat);
}
void screen_task(void *pvParameter){
  printf("Screen Task is started");
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(TOP);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
  for(;;){
    int remainingTimeBudget = ui.update();

    if (remainingTimeBudget > 0) {}
    vTaskDelay(pdMS_TO_TICKS(10));
  } 
}


void battery_timer_callBack( TimerHandle_t xTimer ){
  //calculate the voltage and SOC off the battery
  VBAT = (float)(analogRead(BATTERY_PIN)) / 4095*2*3.3*1.1;
  int VBATmV = (int)(VBAT*1000);
  int indexMin=1;
  //search for interpolation values
  while(batteryCurve[0][indexMin]>VBATmV){
    indexMin++;
  }
  SOC = map(VBATmV,
            batteryCurve[0][indexMin],
            batteryCurve[0][indexMin-1],
            batteryCurve[1][indexMin],
            batteryCurve[1][indexMin-1]
            );
  Serial.print("VBAT: ");
  Serial.println(VBAT);
  Serial.print("VBATmV: ");
  Serial.println(VBATmV);
  Serial.print("SOC: ");
  Serial.println(SOC);

  String topicBattery_state = macAddress + "/status/battery";
  String payload = (String)SOC;
  client.publish(topicBattery_state.c_str(),payload.c_str());
}
