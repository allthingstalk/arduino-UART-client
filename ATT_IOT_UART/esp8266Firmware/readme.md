# About #
This application is the server part that runs on the ESP8266 module itself in order to provide an AT like command set for the ESP8266 WIFI chip.  
This allows client applications to communicate with the WIFI module through the a serial interface (serial communication).

# Installation #
The wifi firmware has to be installed on the module. If you have the grove uart wifi, you can use the following procedure
  1. Create a grove connector to upload the firmware, see: [http://www.seeedstudio.com/wiki/Grove_-_UART_WiFi#Firmware_update](http://www.seeedstudio.com/wiki/Grove_-_UART_WiFi#Firmware_update)
  2. Download the arduino IDE extentions for the ESP8266 in order to upload the sketch to the module. More info here:
		- [https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide)
		- [http://iot-playground.com/2-uncategorised/38-esp8266-and-arduino-ide-blink-example](http://iot-playground.com/2-uncategorised/38-esp8266-and-arduino-ide-blink-example)
  3. Connect the uart wifi module to your pc
  4. Open esp8266Firmware.ino
  5. Press the button on the wifi module until the led goes red, then release (led will dimm a little but remain red)
  6. Compile and upload the ino file
  

# AT commands #
## Format ##
Format = command [param {‘;’ param} \n
## supported commands ##
The system currently supports the following AT commands:

### AT ###
Start wifi
Params: none
Returns: okAT’
### ATW ###
reset wifi
Params: none
Returns: okATW’
###ATI###
Initialize
Params: 
•	deviceId: string, the id of the device.
•	Clientid: string, the id of the client
•	clientKey: string, the client key
Returns: ‘okATI’
###ATC###
Connect to the http server
Params: 
•	http server: string, dns name of the http api server
Returns: ‘okATC’
###ATB###
Subscribe to the broker
Params: 
•	mqtt server: string, dns name of the broker
Returns: ‘okATB’
###ATA###
Add asset
Params:  same as call to ‘addAsset’ for regular lib
Returns: ‘okATA’
###ATS###
Send data over mqtt
Params:  same as call to ‘Send’ for regular lib
Returns: ‘okATS’
###ATR###
Receive data
Params: none
Returns:
When no values: ‘ok’
otherwise:
•	pin nr: string
•	payload: string value

