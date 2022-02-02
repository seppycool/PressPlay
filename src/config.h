#include "wificonfig.h"
#include <EasyButton.h>

#define SCREENACTIVE 0

#define BUTTONLEFT_PIN 19
#define BUTTONLEFT_LED 18
#define BUTTONRIGHT_PIN 22
#define BUTTONRIGHT_LED 21
#define BATTERY_PIN 35

#define STATUS_TIMER_CYCLE 5000
#define QUESTION_TIMER_CYCLE 100


const char* ssid_nerdlab        = SSID_NERDLAB;
const char* password_nerdlab    = PASSWORD_NERDLAB;
const char* mqtt_nerdlab        = MQTT_SERVER_NERDLAB;

const char* ssid_private        = SSID_PRIVATE;
const char* password_private    = PASSWORD_PRIVATE;
const char* mqtt_private        = MQTT_SERVER_PRIVATE;

const char* ssid_pressplay      = SSID_PRESSPLAY;
const char* password_pressplay  = PASSWORD_PRESSPLAY;
const char* mqtt_pressplay      = MQTT_SERVER_PRESSPLAY;

char* mqtt_server = (char*)mqtt_nerdlab;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define BAT_CURVE_COUNT 14
const int batteryCurve[2][BAT_CURVE_COUNT] = { {5000, 4200,  4060, 3980, 3920, 3870, 3820, 3790, 3770, 3740, 3680, 3450, 3000, 0},
                                               {100 , 100 ,    90,   80,   70,   60,   50,   40,   30,   20,   10,    5,    0, 0}};

bool sendStatusUpdateActive = true;
float VBAT;  // battery voltage from ESP32 ADC read
int SOC;  //battery SOC percentage
int SOCAlert = 10;
double RSSI;

String macAddress = WiFi.macAddress();
const int freq = 5000;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int resolution = 8;

int lightshow = 0;
bool buttonsActive = true;
bool buttonsAnimationColorActive = false;
bool ledStripActive = true;

int questionTimerCount = 0;
int questionTimerDuration;
int questionTime = 10000;

#if SCREENACTIVE 
enum Button{e_none,e_buttonLeft,e_buttonRight, e_joystickUp = 5, e_joystickDown, e_joystickLeft, e_joystickRight, e_joystickCenter, e_button_max};
#else
enum Button{e_none,e_buttonLeft,e_buttonRight, e_button_max};
#endif
int buttonCount[e_button_max];
Button lastButtonClicked = e_none;
EasyButton buttonLeft(BUTTONLEFT_PIN);
EasyButton buttonRight(BUTTONRIGHT_PIN);

struct tm timeinfo;

//SCREEN
#if SCREENACTIVE
#define JOYSTICK_UP 33
#define JOYSTICK_DOWN 32
#define JOYSTICK_RIGHT 34
#define JOYSTICK_LEFT 36
#define JOYSTICK_CENTER 39

String screenMQTT = "<=GREEN \n RED=>";
String screenQuestion = "TIME IS \n OVER";

EasyButton joystickLeft(JOYSTICK_LEFT);
EasyButton joystickRight(JOYSTICK_RIGHT);
EasyButton joystickUp(JOYSTICK_UP);
EasyButton joystickDown(JOYSTICK_DOWN);
EasyButton joystickCenter(JOYSTICK_CENTER);

SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui ( &display );
#endif
