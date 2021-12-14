#include "wificonfig.h"
#include <EasyButton.h>

#define BUTTON1_PIN 14
#define BUTTON1_LED 12
#define BUTTON2_PIN 27
#define BUTTON2_LED 25
#define BATTERY_PIN 35


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

int button1LedState = LOW;
int button2LedState = LOW;
enum Button{e_none,e_button1,e_button2};
Button lastButtonClicked = e_none;
EasyButton button1(BUTTON1_PIN);
EasyButton button2(BUTTON2_PIN);

struct tm timeinfo;