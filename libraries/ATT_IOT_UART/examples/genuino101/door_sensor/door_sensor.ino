/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

This sketch is an example sketch to deploy the Grove Magnetic door switch (311070006) to the
AllThingsTalk IoT developer cloud.


### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield, make sure the switch is set to 5.0V
  - Connect USB cable to your computer
  - Connect a Grove Magnetic door switch to pin D2 of the Arduino shield
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
#define switchId 2                // Digital sensor is connected to pin D2 on grove shield

// Required for the device
void callback(int pin, String& value);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc
  Serial.println("Starting sketch");
  Serial1.begin(115200);                               // Init serial link for WiFi module
  while(!Serial1);

  while(!Device.StartWifi())
    Serial.println("Retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))   // If we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("Retrying...");
  while(!Device.Connect(httpServer))                   // Connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("Retrying");

  Device.AddAsset(switchId, "Magnetic door switch", "switch", false, "boolean");   // Create the sensor asset for your device

  delay(1000);                                         // Give the WiFi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud (MQTT). This stops the http connection
    Serial.println("Retrying");

  pinMode(switchId, INPUT);                            // Initialize the digital pin as an input.
  SendValue(digitalRead(switchId));                    // Send initial state
  Serial.println("Magnetic door switch is ready for use!");
}

bool sensorVal;
void loop() 
{
  bool sensorRead = digitalRead(switchId);             // Read status Digital Sensor
  if (sensorVal != sensorRead)                         // Verify if value has changed
  {
     sensorVal = sensorRead;
     SendValue(sensorVal);
  }
  Device.Process();
}

void SendValue(bool value)
{
  if(sensorVal){
    Serial.println("Door closed");
    Device.Send("true", switchId);
  }
  else{
    Serial.println("Door opened");
    Device.Send("false", switchId);
  }
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    Serial.print("Incoming data for: ");         // Display the value that arrived from the AllThingsTalk IOT developer cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.println(value);
    Device.Send(value, pin);                      // Send the value back for confirmation   
}

