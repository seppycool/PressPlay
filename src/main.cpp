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
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"
#include "config.h"
#include "wificonfig.h"

#include "ledStrip.h"

#define TEST 1



// Use the corresponding display class:
void WiFiMqtt_task(void *pvParameter);
void buttons_task(void *pvParameter);
void screen_task(void *pvParameter);
void leds_task(void *pvParameter);
TimerHandle_t StatusTimer_handle;
void status_timer_callBack( TimerHandle_t xTimer );
int getLocalTime();
void setButtonLed(Button button, bool state);
void setButtonLedLastClicked();


void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(BATTERY_PIN, INPUT);

  xTaskCreatePinnedToCore(&WiFiMqtt_task,"WifiMqttTask",4048,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(&buttons_task,"goButtonTask",2048,NULL,5,NULL,1);
  xTaskCreatePinnedToCore(&screen_task,"screenTask",2048,NULL,5,NULL,1);
  xTaskCreatePinnedToCore(&leds_task,"ledsTask",2048,NULL,5,NULL,1);
  StatusTimer_handle = xTimerCreate("StatusTimerTask",pdMS_TO_TICKS(5000),pdTRUE,( void * ) 0,status_timer_callBack);
  if( xTimerStart(StatusTimer_handle, 0 ) != pdPASS )
    Serial.println("Timer not started");
  status_timer_callBack(StatusTimer_handle);
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
  if (String(topic) == "controllers/output/buttonLedLeft" ||
      String(topic) == "controllers/" + macAddress + "/output/buttonLedLeft") {
    int dutycycle = atoi(messageTemp.c_str());
    if(dutycycle > 0 && dutycycle<=255 ){
      ledcWrite(ledChannel1,dutycycle);
    }
  }
  if (String(topic) == "controllers/output/buttonLedRight" ||
      String(topic) == "controllers/" + macAddress + "/output/buttonLedRight") {
    int dutycycle = atoi(messageTemp.c_str());
    if(dutycycle > 0 && dutycycle<=255 ){
      ledcWrite(ledChannel2,dutycycle);
    }
  }
  if (String(topic) == "controllers/output/lightshow" ||
      String(topic) == "controllers/" + macAddress + "/output/lightshow") {
    lightshow = atoi(messageTemp.c_str());
  }
  if (String(topic) == "controllers/output/alertSOC" ||
      String(topic) == "controllers/" + macAddress + "/output/alertSOC") {
    int newAlertSOC = atoi(messageTemp.c_str());
    if(newAlertSOC > 0 && newAlertSOC<=100 )
      SOCAlert = newAlertSOC;
  }
  if (String(topic) == "controllers/output/screen" ||
      String(topic) == "controllers/" + macAddress + "/output/screen") {
    screenMQTT = messageTemp.c_str();
  }
  if (String(topic) == "controllers/output/status" ||
      String(topic) == "controllers/" + macAddress + "/output/status") {
    if(messageTemp == "battery"){
      if(SOC<SOCAlert){
        lightshow = 1;
      }
    }
    if(messageTemp == "lastButton"){
      setButtonLedLastClicked();
    }
    if(messageTemp == "resetLed"){
      setButtonLed(e_buttonLeft,LOW);
      setButtonLed(e_buttonRight,LOW);
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
      client.subscribe("controllers/output/#");

      String topicOutputMacAdress = "controllers/"+ macAddress + "/output/#";
      client.subscribe(topicOutputMacAdress.c_str());

      status_timer_callBack(StatusTimer_handle);

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
  Serial.println("");
  Serial.print(" CONNECTED WITH IP ADRESS: ");
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
void setButtonLed(Button button, bool state){
  switch (button)
  {
  case e_buttonLeft:
    if(state) ledcWrite(ledChannel1, 255);
    else ledcWrite(ledChannel1, 0);
    break;
  case e_buttonRight:
    if(state) ledcWrite(ledChannel2, 255);
    else ledcWrite(ledChannel2, 0);
    break;
  
  default:
    break;
  }
}

void setButtonLedLastClicked(){
  switch (lastButtonClicked)
  {
  case e_buttonLeft:
    setButtonLed(e_buttonLeft, HIGH);
    setButtonLed(e_buttonRight, LOW);
    break;
  case e_buttonRight:
    setButtonLed(e_buttonLeft, LOW);
    setButtonLed(e_buttonRight, HIGH);
    break;
  case e_none:
  default:
    setButtonLed(e_buttonLeft, LOW);
    setButtonLed(e_buttonRight, LOW);
    break;
  }  
}


void startLightShow1(){
  unsigned startTime = millis();
  while((millis()-startTime)<5000){
    ledcWrite(ledChannel1,random(255));
    ledcWrite(ledChannel2,random(255));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  setButtonLedLastClicked();
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
  setButtonLedLastClicked();
}

void sendButtonPressedMqtt(Button buttonPressed){
  if(client.connected()){
    #ifdef TEST
    String topicTest = "controllers/" + macAddress + "/input/button" + (int)buttonPressed;
    String payloadTest = "pressed";
    client.publish(topicTest.c_str(),payloadTest.c_str());
    #endif

    String topic = "controllers/"+ macAddress + "/input";
    String payload = "controllers,id="+macAddress +" button=" + (int)buttonPressed;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/input";
    payload = "buttonPress,id="+macAddress +" button_name=" + (int)buttonPressed;
    client.publish(topic.c_str(),payload.c_str());
  }
  else{
    Serial.println("ERROR: can't send MQTT message --> not connected");
  }
}

void onPressedButtonLeft()
{ 
  Serial.println("Button Left pressed");
  lastButtonClicked = e_buttonLeft;
  setButtonLedLastClicked();
  buttonCount[(int)e_buttonLeft]++;
  ledAnimation = e_glowing;
  ledAnimationColor = CRGB(255,0,0);
  sendButtonPressedMqtt(e_buttonLeft);
}
void onPressedButtonRight()
{ 
  Serial.println("Button Right pressed");
  lastButtonClicked = e_buttonRight;
  setButtonLedLastClicked();
  buttonCount[(int)e_buttonRight]++;

  ledAnimation = e_SpinningSinWave;
  ledAnimationColor = CRGB(0,255,0);

  sendButtonPressedMqtt(e_buttonRight);
}

void onPressedJoystickLeft(){
  buttonCount[(int)e_joystickLeft]++;
  sendButtonPressedMqtt(e_joystickLeft);
}
void onPressedJoystickRight(){
  buttonCount[(int)e_joystickRight]++;
  sendButtonPressedMqtt(e_joystickRight);
}
void onPressedJoystickUp(){
  buttonCount[(int)e_joystickUp]++;
  sendButtonPressedMqtt(e_joystickUp);
}
void onPressedJoystickDown(){
  buttonCount[(int)e_joystickDown]++;
  sendButtonPressedMqtt(e_joystickDown);
}
void onPressedJoystickCenter(){
  buttonCount[(int)e_joystickCenter]++;
  sendButtonPressedMqtt(e_joystickCenter);

  #ifdef TEST
    lastButtonClicked = e_none;
    setButtonLedLastClicked();
  #endif
  ledAnimation = e_pride;
}

void buttons_task(void *pvParameter){
  printf("button task is started");

  //LEFT
  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(BUTTONLEFT_LED, ledChannel1);
  ledcWrite(ledChannel1,0);
  buttonLeft.begin();
  buttonLeft.onPressed(onPressedButtonLeft);

  //RIGHT
  ledcSetup(ledChannel2, freq, resolution);
  ledcAttachPin(BUTTONRIGHT_LED, ledChannel2);
  ledcWrite(ledChannel2,0);
  buttonRight.begin();
  buttonRight.onPressed(onPressedButtonRight);

  //JOYSTICK LEFT
  joystickLeft.begin();
  joystickLeft.onPressed(onPressedJoystickLeft);

  //JOYSTICK RIGHT
  joystickRight.begin();
  joystickRight.onPressed(onPressedJoystickRight);

  //JOYSTICK UP
  joystickUp.begin();
  joystickUp.onPressed(onPressedJoystickUp);

  //JOYSTICK DOWN
  joystickDown.begin();
  joystickDown.onPressed(onPressedJoystickDown);

  //JOYSTICK CENTER
  joystickCenter.begin();
  joystickCenter.onPressed(onPressedJoystickCenter);

  for(;;){
    if(buttonsActive){
      buttonLeft.read();
      buttonRight.read();
      joystickLeft.read();
      joystickRight.read();
      joystickUp.read();
      joystickDown.read();
      joystickCenter.read();
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
void MQTTFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { analogClockFrame, digitalClockFrame, batteryFrame, MQTTFrame};

// how many frames are there?
int frameCount = 4;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;

int getLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    //Serial.println("Failed to obtain time");
    return 1;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return 0;
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
  display->drawString(clockCenterX + x , 16 + y, socBat);
  display->drawString(clockCenterX + x , clockCenterY + y, vBat );
}

void MQTTFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , 16 + y, screenMQTT);
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


void status_timer_callBack( TimerHandle_t xTimer ){
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

  if(client.connected()){
    #ifdef TEST
    String topic = "controllers/" + macAddress + "/diagnostic/battery";
    String payload = (String)SOC;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/" + macAddress + "/diagnostic/lastClicked";
    payload = (int)lastButtonClicked;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/diagnostic/im_alive";
    payload = (int)(millis());
    client.publish(topic.c_str(),payload.c_str());

    for(int i = e_none; i< e_button_max; i++){
      topic = "controllers/" + macAddress + "/diagnostic/buttonCount"+ i;
      payload = (int)buttonCount[i];
      client.publish(topic.c_str(),payload.c_str());
    }
    #endif

    topic = "controllers/"+ macAddress + "/diagnostic";
    payload = "controllers,id="+macAddress +" SOC=" + (String)SOC;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/diagnostic";
    payload = "controllers,id="+macAddress +" lastClicked=" + (int)lastButtonClicked;
    client.publish(topic.c_str(),payload.c_str());

    for(int i = e_none; i< e_button_max; i++){
      topic = "controllers/" + macAddress + "/diagnostic";
      payload = "controllers,id=" + macAddress + " buttonCount" + (int)i + "=" + (int)buttonCount[i];
      client.publish(topic.c_str(),payload.c_str());
    }

    topic = "controllers/"+ macAddress + "/im_alive";
    payload = "controllers,id="+macAddress +" active=" + (int)(millis()/1000);
    client.publish(topic.c_str(),payload.c_str());
  }
}


void leds_task(void *pvParameter){
  vTaskDelay(pdMS_TO_TICKS(3000)); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(ledStrip, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  for(;;)
  {
    switch (ledAnimation)
    {
    case e_glowing:
      //SpinningSinWave(CRGB(255,0,0),4);
      //vTaskDelay(pdMS_TO_TICKS(100));
      glowing(ledAnimationColor,5);
      break;
    case e_SpinningSinWave:
      //CenterToRight(0,255,0);
      //setPixels(CRGB(0,255,0),0, CENTER_LED);
      SpinningSinWave(ledAnimationColor,4);
      vTaskDelay(pdMS_TO_TICKS(100));
      break;
    case e_pride:
      pride();
      FastLED.show();
    case e_allOn:
      setPixels(ledAnimationColor,0,NUM_LEDS);
      break;
    case e_allOff:
      setPixels(CRGB(0,0,0),0,NUM_LEDS);
      break;
    default:
      //setPixels(CRGB(0,0,0),0,NUM_LEDS);
      pride();
      FastLED.show();
      break;
    }    
    vTaskDelay(pdMS_TO_TICKS(1000/FRAMES_PER_SECOND));
  }
}




