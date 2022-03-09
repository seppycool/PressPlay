# PressPlay
## This is the code for the controller that will be used by pressPlay

This controller has 2 buttons and a OLED Screen
This will connect to wifi and MQTT to send all data

# ButtonNumbers

| Number | Button
:--- | :---
1 | Button Left
2 | Button Right
5 | Joystick Up
6 | Joystick Down
7 | Joystick Left
8 | Joystick Right
9 | Joystick Center

# Animations

| Number | Animation
:--- | :---
0 | All off
1 | Cyclon
2 | Cyclon 2
3 | Cyclon Left
4 | Cyclon Right
5 | One Glow
6 | One Glow With RSSI Delay
7 | Glowing
8 | Spinning Sin Wave
9 | Pride
10 | All on
11 | Question Clock
12 | Twinkle
13 | Confetti
14 | Rainbow
15 | Rainbow Glitter 
16 | All on Glitter


# Subscriptions

| Topic | Payload | Description
:--- | :--- | :---
controllers/output/buttonLedLeft | dutycycle (0-255) | Set dutycycle left button going from 0/off to 255/on
controllers/output/buttonLedRight | dutycycle (0-255) | Set dutycycle right button led going from 0/off to 255/on
controllers/output/lightshow | lightshow number (int) | Start lightshow
controllers/output/alertSOC | alertSOC (0-100)% | Set threshold SOC for low battery
controllers/output/questionTime | question Duration  (ms) | Set duration of the questions in ms
controllers/output/ledStrip/brightness | brightness (0-255) | set the brightness of the ledstrip.
controllers/output/ledStrip/color | colorValue (000000-FFFFFF) | set the animation color of the ledstrip.
controllers/output/ledStrip/animation | animation number (0-15) | set the animation of the ledstrip.
controllers/output/ledStrip/delay | delay (int) | set the delay for the currrent animation.
controllers/output/screen | Screentext (String) | Set a text on the screen of the controller
controllers/output/status | "statusUpdateActivate" | deactivate status update (i am alive keeps active)
controllers/output/status | "statusUpdateDeactivate" | activate status update
controllers/output/status | "sendUpdate" | update status message
controllers/output/status | "battery" | lightup the controller if SOC is lower than alertSOC
controllers/output/status | "lastButton" | lightup the last pressed button
controllers/output/status | "resetLed" | set both buttonleds to off state
controllers/output/status | "buttonsDeactivate" | deactivate all buttons
controllers/output/status | "buttonsActivate" | activate all buttons
controllers/output/status | "buttonsAnimationColorDeactivate" | disable switching color depending on buttons pressed
controllers/output/status | "buttonsAnimationColorActivate" | enable switching color depending on buttons pressed
controllers/output/status | "ledStripDeactivate" | deactivate ledstrip
controllers/output/status | "ledsStripActivate" | activate ledstrip
controllers/output/status | "ledStripDeactivate" | deactivate ledstrip
controllers/output/status | "ledsStripActivate" | activate ledstrip
controllers/output/status | "demoModeDeactivate" | deactivate demoMode
controllers/output/status | "demoModeActivate" | activate demoMode
controllers/output/status | "confirmDeactivate" | deactivate confirm message after receiving
controllers/output/status | "confirmActivate" | activate confirm message after receiving
controllers/[macAdress]/output/# | # | same behaviour as other but specific for a controller with macAdress

# Publications

| Topic | Payload | trigger |description
--- | --- | --- | ---
controllers/[macAdress]/input | "buttonPress,id=[macAdress] button_name=[buttonNumber]" | buttonPress | Message will be send if button with buttonNumber is pressed
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] SOC=[SOC]" | 5 sec timer | the SOC of the controller
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] lastClicked=[buttonNumber]" | 5 sec timer | last Pressed button
controllers/[macAdress]/diagnostic | "controllers,id=[macAdress] buttonCount[buttonNumber]=[#buttonPressed]" | 5 sec timer | total amount of presses for every button
controllers/[macAdress]/im_alive | "controllers,id=[macAdress] active=[secActive]" | 5 sec timer | im alive message with active time in seconds
controllers/[macAdress]/confirm/[Recieved Topic] | "[Recieved Message]" | confirmActivate && "receive message" | A confirm message will be send with same recieved topic and message. This only when confirm message is activated


