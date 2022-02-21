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

#include "FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <EasyButton.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <HttpsOTAUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

String FirmwareVer = {
  "1.0"
};
#define URL_fw_Version "https://raw.githubusercontent.com/seppycool/PressPlay/master/OTA/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/seppycool/PressPlay/master/OTA/firmware.bin"

// Include custom images
#include "images.h"
#include "config.h"
#include "wificonfig.h"

#include "ledStrip.h"
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier

#if SCREENACTIVE
// Include the correct display library
// Include the UI lib
// For a connection via I2C using Wire include
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "OLEDDisplayUi.h"
#endif


#define TEST 1
#define DEBUG 0

// Use the corresponding display class:
void WiFiMqtt_task(void *pvParameter);
void buttons_task(void *pvParameter);
#if SCREENACTIVE
void screen_task(void *pvParameter);
#endif
void leds_task(void *pvParameter);
TimerHandle_t StatusTimer_handle;
void status_timer_callBack( TimerHandle_t xTimer );
TimerHandle_t QuestionTimer_handle;
void question_timer_callBack( TimerHandle_t xTimer );
void initQuestionTimer(int duration, int waitTime = 0);
int getLocalTime();
void setButtonLed(Button button, bool state);
void setButtonLedLastClicked();
void sendStatusUpdate();
int calculateSOC();


//AsyncWebServer server(80);


void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(BATTERY_PIN, INPUT);
  //Calculate Starting SOC
  SOC = calculateSOC();

  xTaskCreatePinnedToCore(&WiFiMqtt_task,"WifiMqttTask",8000/*4048*/,NULL,5,NULL,1);
  xTaskCreatePinnedToCore(&buttons_task,"buttonTask",2048,NULL,1,NULL,1);
  #if SCREENACTIVE
  xTaskCreatePinnedToCore(&screen_task,"screenTask",2048,NULL,1,NULL,1);
  #endif
  xTaskCreatePinnedToCore(&leds_task,"ledsTask",2048,NULL,1,NULL,1);
  StatusTimer_handle = xTimerCreate("StatusTimerTask",pdMS_TO_TICKS(STATUS_TIMER_CYCLE),pdTRUE,( void * ) 0,status_timer_callBack);
  QuestionTimer_handle = xTimerCreate("QuestionTimerTask",pdMS_TO_TICKS(QUESTION_TIMER_CYCLE),pdTRUE,( void * ) 1,question_timer_callBack);
  if( xTimerStart(StatusTimer_handle, 0 ) != pdPASS )
    Serial.println("Timer not started");
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
    if(dutycycle >= 0 && dutycycle<=255 ){
      ledcWrite(ledChannel1,dutycycle);
    }
  }
  if (String(topic) == "controllers/output/buttonLedRight" ||
      String(topic) == "controllers/" + macAddress + "/output/buttonLedRight") {
    int dutycycle = atoi(messageTemp.c_str());
    if(dutycycle >= 0 && dutycycle<=255 ){
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
    if(newAlertSOC >= 0 && newAlertSOC<=100 )
      SOCAlert = newAlertSOC;
  }
  if (String(topic) == "controllers/output/questionTime" ||
      String(topic) == "controllers/" + macAddress + "/output/questionTime") {
    int newQuestionTime = atoi(messageTemp.c_str());
    if(newQuestionTime >= 0)
      questionTime = newQuestionTime;
  }
  if (String(topic) == "controllers/output/ledStrip/brightness" ||
      String(topic) == "controllers/" + macAddress + "/output/ledStrip/brightness") {
    int brightness = atoi(messageTemp.c_str());
    if(brightness >= 0 && brightness<=255 )
      FastLED.setBrightness(brightness);
  }
  if (String(topic) == "controllers/output/ledStrip/color" ||
      String(topic) == "controllers/" + macAddress + "/output/ledStrip/color") {
    long ledColor = strtol(messageTemp.c_str(), NULL, 16);
    if(ledColor >= 0 && ledColor <= 0xFFFFFF)
       ledAnimationColor = CRGB(ledColor);
  }
  if (String(topic) == "controllers/output/ledStrip/animation" ||
      String(topic) == "controllers/" + macAddress + "/output/ledStrip/animation") {
    int Animation = atoi(messageTemp.c_str());
    if(Animation >= 0 && Animation < (int)e_ledAnimations_max){
      if(Animation == (int)e_questionClock || Animation == (int)e_oneGlow){
        initQuestionTimer(questionTime);
      }
      if(Animation == (int)e_oneGlowRSSIDelay){
        animationStartDelay = (WiFi.RSSI()*-250);
        initQuestionTimer(questionTime);
      }
      ledAnimation = (LedAnimation)Animation;
    }
       
  }
  if (String(topic) == "controllers/output/ledStrip/delay" ||
      String(topic) == "controllers/" + macAddress + "/output/ledStrip/delay") {
    int ledDelay = atoi(messageTemp.c_str());
    if(ledDelay > 0)
       animationDelay[(int)ledAnimation] = ledDelay;
  }
  
  #if SCREENACTIVE
  if (String(topic) == "controllers/output/screen" ||
      String(topic) == "controllers/" + macAddress + "/output/screen") {
    screenMQTT = messageTemp.c_str();
  }
  #endif
  if (String(topic) == "controllers/output/status" ||
      String(topic) == "controllers/" + macAddress + "/output/status") {
    if(messageTemp == "statusUpdateActivate"){
      sendStatusUpdateActive = true;
    }
    if(messageTemp == "statusUpdateDeactivate"){
      sendStatusUpdateActive = false;
    }
    if(messageTemp == "sendUpdate"){
      sendStatusUpdate();
    }
    if(messageTemp == "battery"){
      if(SOC<SOCAlert){
        lightshow = 1;
        ledAnimationColor = CRGB::Red;
        ledAnimation = e_glowing;
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
    if(messageTemp == "buttonsAnimationColorDeactivate"){
      buttonsAnimationColorActive = false;
    }
    if(messageTemp == "buttonsAnimationColorActivate"){
      buttonsAnimationColorActive = true;
    }
    if(messageTemp == "ledStripDeactivate"){
      ledStripActive = false;
    }
    if(messageTemp == "ledsStripActivate"){
      ledStripActive = true;
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
      client.subscribe("controllers/output/ledStrip/#");

      String topicOutputMacAdress = "controllers/"+ macAddress + "/output/#";
      client.subscribe(topicOutputMacAdress.c_str());
      topicOutputMacAdress = "controllers/"+ macAddress + "/output/ledStrip/#";
      client.subscribe(topicOutputMacAdress.c_str());

      status_timer_callBack(StatusTimer_handle);
      sendStatusUpdate();

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

int FirmwareVersionCheck();
void firmwareUpdate();

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("");
  Serial.print(" CONNECTED WITH IP ADRESS: ");
  Serial.println(WiFi.localIP());
  Serial.print( "RSSI: ");
  RSSI = WiFi.RSSI();
  Serial.println(RSSI);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid_pressplay, password_pressplay);
}

void WiFiMqtt_task(void *pvParameter){
  //connect to WiFi
  mqtt_server = (char*)mqtt_pressplay;

  //Set >ifi Callback events
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.printf("Connecting to %s ", ssid_pressplay);
  WiFi.begin(ssid_pressplay, password_pressplay);
  int connectionCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
      connectionCount++;
      vTaskDelay(pdMS_TO_TICKS(500));
      Serial.print(".");

      if(connectionCount==10){
        mqtt_server = (char*)mqtt_private;
        Serial.printf("Connecting to %s ", ssid_private);
        WiFi.begin(ssid_private, password_private);
      }
#if DEBUG
      if(connectionCount==20){
        mqtt_server = (char*)mqtt_nerdlab;
        Serial.printf("Connecting to %s ", ssid_nerdlab);
        WiFi.begin(ssid_nerdlab, password_nerdlab);
      }
#endif

  }
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //  request->send(200, "text/plain", "Hi! I am ESP32.");
  //});
  //AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  //server.begin();
  //Serial.println("HTTP server started");
  if(FirmwareVersionCheck()){
    firmwareUpdate();
  }


  for(;;){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  } 
}

void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate2);
  //HttpsOTA.begin(URL_fw_Bin, rootCACertificate);
  //httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);
  /*while(HttpsOTA.status() == HTTPS_OTA_UPDATING){
    Serial.println("Updating Software");
  }
  if(HttpsOTA.status() == HTTPS_OTA_SUCCESS){
    Serial.println("update Succesfull, reboot system");
    ESP.restart();
  }
  if(HttpsOTA.status() == HTTPS_OTA_FAIL){
    Serial.println("update Failed");
  }*/

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

int FirmwareVersionCheck(void) {
  String payload;
  int httpCode = 0;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate2);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      vTaskDelay(pdMS_TO_TICKS(100));
      httpCode = https.GET();
      vTaskDelay(pdMS_TO_TICKS(100));
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
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

    // String topic = "controllers/"+ macAddress + "/input";
    // String payload = "controllers,id="+macAddress +" button=" + (int)buttonPressed;
    // client.publish(topic.c_str(),payload.c_str());

    // topic = "controllers/"+ macAddress + "/input";
    // payload = "buttonPress,id="+macAddress +" button_name=" + (int)buttonPressed;
    // client.publish(topic.c_str(),payload.c_str());
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

  if(buttonsAnimationColorActive)
    ledAnimationColor = CRGB::Yellow;
  
  if(demoMode){
    ledAnimation =  (LedAnimation)(buttonCount[(int)e_buttonLeft]%e_ledAnimations_max);
    if(ledAnimation == e_questionClock || ledAnimation == e_oneGlow){
      initQuestionTimer(questionTime);
    }
  }
 
  sendButtonPressedMqtt(e_buttonLeft);
}
void onPressedButtonRight()
{ 
  Serial.println("Button Right pressed");
  lastButtonClicked = e_buttonRight;
  setButtonLedLastClicked();
  buttonCount[(int)e_buttonRight]++;

  if(buttonsAnimationColorActive)
    ledAnimationColor = CRGB::Blue;
  
  if(demoMode){
    ledAnimationColor = ledAnimationColor.setHue((buttonCount[(int)e_buttonRight]*25)%255);
  }

  // ledAnimation = e_questionClock;
  // ledAnimationColor = CRGB::Blue;
  // initQuestionTimer(questionTime);
  sendButtonPressedMqtt(e_buttonRight);
}

#if SCREENACTIVE
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
}
#endif

void buttons_task(void *pvParameter){
  printf("button task is started");

  //LEFT
  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(BUTTONLEFT_LED, ledChannel1);
  ledcWrite(ledChannel1,0);
  buttonLeft.begin();
  buttonLeft.onPressed(onPressedButtonLeft);
  
  //enable Demo Mode
  if(buttonLeft.isPressed()){
    demoMode = true;
    buttonsActive = true;
    ledAnimation = e_rainbow;
  }

  //RIGHT
  ledcSetup(ledChannel2, freq, resolution);
  ledcAttachPin(BUTTONRIGHT_LED, ledChannel2);
  ledcWrite(ledChannel2,0);
  buttonRight.begin();
  buttonRight.onPressed(onPressedButtonRight);


#if SCREENACTIVE
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
#endif

  for(;;){
    if(buttonsActive){
      buttonLeft.read();
      buttonRight.read();
#if SCREENACTIVE
      joystickLeft.read();
      joystickRight.read();
      joystickUp.read();
      joystickDown.read();
      joystickCenter.read();
#endif
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
int getLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    //Serial.println("Failed to obtain time");
    return 1;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return 0;
}

void question_timer_callBack( TimerHandle_t xTimer ){
  questionTimerCount +=QUESTION_TIMER_CYCLE;
  if(questionTimerCount>=questionTimerDuration) xTimerStop(QuestionTimer_handle,0); 
}

void initQuestionTimer(int duration, int waitTime){
  questionTimerCount = 0;
  questionTimerDuration = duration;
  if( xTimerStart(QuestionTimer_handle, pdMS_TO_TICKS(waitTime) ) != pdPASS )
    Serial.println("Timer not started");
}


#if SCREENACTIVE
//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// SCREEN /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the OLED display using Wire library
void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void batteryFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void MQTTFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void questionFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void questionFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { analogClockFrame, digitalClockFrame, batteryFrame, MQTTFrame, questionFrame,questionFrame2};

// how many frames are there?
int frameCount = 6;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { clockOverlay };
int overlaysCount = 1;



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
  display->drawString(clockCenterX + x , 10 + y, socBat);
  display->drawString(clockCenterX + x , clockCenterY + y, vBat );
}

void MQTTFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , 10 + y, screenMQTT);
}
void questionFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int stopAngle = map(questionTimerCount,0,questionTimerDuration,0,360);
  if(questionTimerCount<questionTimerDuration){
    display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
    for ( int z = 0; z < stopAngle; z++ ) {
      //Begin at 0° and stop at 360°
      float angle = z ;
      angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
      int x2 = ( clockCenterX + ( sin(angle) * clockRadius ) );
      int y2 = ( clockCenterY - ( cos(angle) * clockRadius ) );
      int x3 = ( clockCenterX );
      int y3 = ( clockCenterY );
      display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
    }
  }
  else{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_24);
    display->drawString(clockCenterX + x , 10 + y, screenQuestion);
  }
}

void questionFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int stopWidth = map(questionTimerCount,0,questionTimerDuration,0,screenW);
  if(questionTimerCount<questionTimerDuration){
    for ( int w = 0; w < stopWidth; w++ ) {
      int x2 = ( w );
      int y2 = ( 16);
      int x3 = ( w );
      int y3 = ( screenH );
      display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
    }
  }
  else{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_24);
    display->drawString(clockCenterX + x , 10 + y, screenQuestion);
  }
}

void screen_task(void *pvParameter){
  printf("Screen Task is started");
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

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

  ui.disableAutoTransition();

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  for(;;){
    int remainingTimeBudget = ui.update();

    if (remainingTimeBudget > 0) {}
    vTaskDelay(pdMS_TO_TICKS(10));
  } 
}
#endif

void sendStatusUpdate(){
  if(client.connected()){
    String topic = "controllers/" + macAddress + "/diagnostic/IP";
    String payload = (String)WiFi.localIP().toString();
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/" + macAddress + "/diagnostic/RSSI";
    payload = (String)((int)RSSI);
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/" + macAddress + "/diagnostic/lastClicked";
    payload = (int)lastButtonClicked;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/diagnostic/ledColor";
    int ledColorRed = ledAnimationColor.r;
    int ledColorGreen = ledAnimationColor.g;
    int ledColorBlue = ledAnimationColor.b;
    int ledColor = (ledColorRed<<16) + (ledColorGreen<<8) + ledColorBlue;
    payload = (int)(ledColor);
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/diagnostic/ledAnimation";
    payload = (int)(ledAnimation);
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/"+ macAddress + "/diagnostic/ledDelay";
    payload = (int)(ledAnimationDelay);
    client.publish(topic.c_str(),payload.c_str());

    for(int i = e_none; i< e_button_max; i++){
      topic = "controllers/" + macAddress + "/diagnostic/buttonCount"+ i;
      payload = (int)buttonCount[i];
      client.publish(topic.c_str(),payload.c_str());
    }
  }
}

int calculateSOC(){
  //calculate the voltage and SOC off the battery
  VBAT = (float)(analogRead(BATTERY_PIN)) / 4095*2*3.3*1.1;
  Serial.print("Battery voltage: ");
  Serial.println(VBAT);
  int VBATmV = (int)(VBAT*1000);
  int indexMin=1;
  //search for interpolation values
  while(batteryCurve[0][indexMin]>VBATmV){
    indexMin++;
  }

  int SOCCalculated = map( VBATmV,
                                batteryCurve[0][indexMin],
                                batteryCurve[0][indexMin-1],
                                batteryCurve[1][indexMin],
                                batteryCurve[1][indexMin-1]
                                );
  return SOCCalculated;
}

void status_timer_callBack( TimerHandle_t xTimer ){
  SOC = SOC*0.9 +  0.1 * calculateSOC();
  Serial.print("SOC: ");
  Serial.println(SOC);
  RSSI = RSSI*0.95 + WiFi.RSSI()*0.05;

  if(client.connected()){
    String topic = "controllers/"+ macAddress + "/diagnostic/im_alive";
    int activeTime = millis();
    String payload = (String)activeTime;
    client.publish(topic.c_str(),payload.c_str());

    topic = "controllers/" + macAddress + "/diagnostic/battery";
    payload = (String)SOC;
    client.publish(topic.c_str(),payload.c_str());
  }

  if(sendStatusUpdateActive){
    sendStatusUpdate();
  }
}


void leds_task(void *pvParameter){
  vTaskDelay(pdMS_TO_TICKS(3000)); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(ledStrip, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  for(;;)
  {
    if(ledStripActive){
      switch (ledAnimation)
      {
      case e_allOff:
        setPixels(CRGB(0,0,0),0,NUM_LEDS);
        FastLED.show();
        break;
      case e_cyclon:
        cyclon(CENTER_LED, NUM_LEDS,ledAnimationColor);
        break;
      case e_cyclon2:
        cyclonMiddle(0, NUM_LEDS,ledAnimationColor);
        break;
      case e_cyclonLeft:
        cyclon(CENTER_LED, NUM_LEDS, CRGB::Yellow);
        break;
      case e_cyclonRight:
        cyclon(0, CENTER_LED, CRGB::Blue);
        break;
      case e_oneGlowRSSIDelay:
        if(animationStartDelay){
          setPixels(CRGB(0,0,0),0,NUM_LEDS);
          FastLED.show();
          animationStartDelay -= animationDelay[(int)ledAnimation];
          if(animationStartDelay<=0){
            initQuestionTimer(questionTime);
          } 
        }
        else{
          oneGlow(ledAnimationColor, questionTimerCount, questionTimerDuration);
        }
        break;
      case e_oneGlow:
        oneGlow(ledAnimationColor, questionTimerCount, questionTimerDuration);
        break;
      case e_glowing:
        glowing(ledAnimationColor);
        break;
      case e_SpinningSinWave:
        SpinningSinWave(ledAnimationColor,3);
        break;
      case e_pride:
        pride();
        FastLED.show();
        break;
      case e_allOn:
        setPixels(ledAnimationColor,0,NUM_LEDS);
        FastLED.show();
        break;
      case e_questionClock:
        if(questionTimerCount<questionTimerDuration) questionClock(ledAnimationColor, questionTimerCount, questionTimerDuration);
        else{
#if DEBUG
          pride();
#else
          setAll(CRGB::Black);
#endif
          FastLED.show();
        }
        break;
      case e_twinkle:
        drawTwinkles();
        FastLED.show();
        break;
      case e_confetti:
        confetti();
        FastLED.show();
        break;
      case e_rainbow:
        rainbow();
        FastLED.show();
        break;
      case e_rainbowGlitter:
        rainbowWithGlitter();
        FastLED.show();
        break;
      case e_allOnGlitter:
        setPixels(ledAnimationColor,0,NUM_LEDS);
        addGlitter(80);
        FastLED.show();
      default:
        pride();
        FastLED.show();
        break;
      }    
      vTaskDelay(pdMS_TO_TICKS(animationDelay[(int)ledAnimation]));
    }
    else{
      setPixels(CRGB(0,0,0),0,NUM_LEDS);
      FastLED.show();
      vTaskDelay(pdMS_TO_TICKS(100));
    }

  }
}




