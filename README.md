# PressPlay
## This is the code for the controller that will be used by pressPlay

This controller has 2 buttons and a OLED Screen
This will connect to wifi and MQTT to send all data

# ButtonNumbers
| Number | Button
--- | ---
1 | Button Left
2 | Button Right
5 | Joystick Up
6 | Joystick Down
7 | Joystick Left
8 | Joystick Right
9 | Joystick Center


# Subscriptions
| Topic | Payload | Description
--- | --- | ---
controllers/output/buttonLedLeft | dutycycle (0-255) | Set dutycycle left button going from 0/off to 255/on
controllers/output/buttonLedRight | dutycycle (0-255) | Set dutycycle right button led going from 0/off to 255/on
controllers/output/lightshow | lightshow number (int) | Start lightshow
controllers/output/alertSOC | alertSOC (0-100)% | Set threshold SOC for low battery
controllers/output/screen | Screentext (String) | Set a text on the screen of the controller
controllers/output/status | "battery" | lightup the controller if SOC is lower than alertSOC
controllers/output/status | "lastButton" | lightup the last pressed button
controllers/output/status | "resetLed" | set both buttonleds to off state
controllers/output/status | "buttonsDeactivate" | deactivate all buttons
controllers/output/status | "buttonsActivate" | activate all buttons
controllers/[macAdress]/output/# | # | same behaviour as other but specific for a controller with macAdress

# Publications
| Topic | Payload | trigger |description
--- | --- | --- | ---
controllers/[macAdress]/input | "buttonPress,id=[macAdress] button_name=[buttonNumber]" | buttonPress | Message will be send if button with buttonNumber is pressed
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] SOC=[SOC]" | 5 sec timer | the SOC of the controller
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] lastClicked=[buttonNumber]" | 5 sec timer | last Pressed button
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] buttonCount[buttonNumber]=[#buttonPressed]" | 5 sec timer | total amount of presses for every button
controllers/[macAdress]/im_alive | "controllers,id=[macAdress] active=[secActive]" | 5 sec timer | im alive message with active time in seconds


