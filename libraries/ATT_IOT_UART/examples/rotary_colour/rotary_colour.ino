/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

This sketch is an example sketch to deploy the Grove Rotary angle sensor (101020017) to the
AllThingsTalk IoT developer cloud.


### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield, make sure the switch is set to 3.3V
  - Connect USB cable to your computer
  - Connect a Grove Rotary angle sensor to pin A2 of the Arduino shield
  - Grove UART wifi to pin UART (D0,D1)
2. Add 'ATT_IOT_UART' library to your Arduino Environment
     More info can be found at http://arduino.cc/en/Guide/Libraries
3. Fill in the missing strings (deviceId, clientId and clientKey) in the keys.h file
4. Optionally, change sensor names, labels as appropiate
5. Upload the sketch

Note: for use of extra actuators, extend the callback function at the end of the sketch

*/

#include "ATT_IOT_UART.h"      // AllThingsTalk Arduino UART IoT library
#include <SPI.h>               // Required to have support for signed/unsigned long type
#include "keys.h"              // Keep all your personal account information in a seperate file

ATTDevice Device(&Serial1);
char httpServer[] = "api.smartliving.io";          // HTTP API Server host
char mqttServer[] = "broker.smartliving.io";       // MQTT Server Address

// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define rotaryId 2                // Analog sensor is connected to pin A2 on grove shield
#define rotaryColor 3             // Extra asset to show colours in the SmartLiving dashboard

// Required for the device
void callback(int pin, String& value);

void setup()
{
  pinMode(rotaryId, INPUT);                            // Initialize the digital pin as an input.
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial) ;                                     // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting sketch");
  Serial1.begin(115200);                               // Init serial link for WiFi module
  while(!Serial1);

  while(!Device.StartWifi())
    Serial.println("Retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))   // If we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("Retrying...");
  while(!Device.Connect(httpServer))                   // Connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("Retrying");

  Device.AddAsset(rotaryId, "Rotary angle sensor", "rotary", false, "integer");   // Create the sensor asset for your device
  Device.AddAsset(rotaryColor, "Colour", "Hot-Cold", false, "string");

  delay(1000);                                         // Give the WiFi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud (MQTT). This stops the http connection
    Serial.println("Retrying");

  Serial.println("Rotary angle sensor is ready for use!");
}

int sensorValAvg = 0;
int sensorReadAvg;
int sampleRate = 50;

unsigned int red, blue;

void loop()
{
  // Calculate average of several values to compensate for sensor wobbling
  sensorReadAvg = 0;
  for(int i=0; i<sampleRate; i++) {
    sensorReadAvg += analogRead(rotaryId);
  }
  sensorReadAvg /= sampleRate;

  // Verify if average value has change
  if (abs(sensorValAvg - sensorReadAvg) > 1)
  {
    sensorValAvg = sensorReadAvg;
    if(sensorValAvg>1020) sensorValAvg = 1020;     // Cap at 1020 (instead of the 1023 max) as we want to map to colours and 1020 = 4x 255

    // Set rgb value for red and blue
    blue = 255 - sensorValAvg / 4;              // 255,0,0 -> 128,0,128 -> 0,0,255
    red = sensorValAvg / 4;

    // Compose string representing the colour in hexadecimal
    String redstr = red <= 15 ? "0" + String(red,HEX) : String(red,HEX);
    String bluestr = blue <= 15 ? "0" + String(blue,HEX) : String(blue,HEX);
    String color = "#" + redstr + "00" + bluestr;

    // Send value for both assets to the platform
    Device.Send(String(sensorValAvg), rotaryId);
    Device.Send(color, rotaryColor);
  }
  Device.Process();
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
}

