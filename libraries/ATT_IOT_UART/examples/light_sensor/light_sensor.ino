/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

This sketch is an example sketch to deploy the Grove Light sensor (101020014) to the
AllThingsTalk IoT developer cloud.


### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield, make sure the switch is set to 3.3V
  - Connect USB cable to your computer
  - Connect a Grove Light sensor to pin A0 of the Arduino shield
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

// Fixed constants for minimum and maximum sensor value
#define sensorMin 0
#define sensorMax 1023

#define ARRAYSIZE 6
String light_Range[ARRAYSIZE] = { "dark", "moonlight", "sunset", "cloudy", "daylight", "sunlight" };


ATTDevice Device(&Serial1);
char httpServer[] = "api.smartliving.io";          // HTTP API Server host
char mqttServer[] = "broker.smartliving.io";       // MQTT Server Address

// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define lightId 0                // Analog sensor is connected to pin A0 on grove shield

// Required for the device
void callback(int pin, String& value);

void setup()
{
  pinMode(lightId, INPUT);                       // Initialize the digital pin as an input.
  Serial.begin(57600);                           // Init serial link for debugging
  while(!Serial) ;                               // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting sketch");
  Serial1.begin(115200);                         // Init serial link for WiFi module
  while(!Serial1);

  while(!Device.StartWifi())
    Serial.println("Retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))   // If we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("Retrying...");
  while(!Device.Connect(httpServer))                   // Connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("Retrying");

  Device.AddAsset(lightId, "Light sensor", "light", false, "string");   // Create the sensor asset for your device

  delay(1000);                                         // Give the WiFi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud (MQTT). This stops the http connection
    Serial.println("Retrying");

  Serial.println("Light sensor is ready for use!");
}

int Prev_range = 0;
void loop()
{
  int sensorRead = analogRead(lightId);                 // Read status Analog Sensor
  // Serial.println(sensorRead);
  // Map the sensor range to a range of six options
  int range = map(sensorRead, sensorMin, sensorMax, 0, 5);
  if (range != Prev_range)                                  // Verify if value has changed
  {
    Serial.println(light_Range[range]);
    Device.Send(light_Range[range], lightId);
    Prev_range = range;
  }
  Device.Process(); 
  delay(1000);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
  Serial.print("Incoming data for: ");      // Display the value that arrived from the AllThingsTalk IOT developer cloud.
  Serial.print(pin);
  Serial.print(", value: ");
  Serial.println(value);
  Device.Send(value, pin);                  // Send the value back for confirmation   
}

