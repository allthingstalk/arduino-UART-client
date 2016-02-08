/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

This sketch is an example sketch to deploy the Grove Air quality sensor v1.3 (101020078) to the
AllThingsTalk IoT developer cloud.


### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield, make sure the switch is set to 3.3V
  - Connect USB cable to your computer
  - Connect a Grove Air quality sensor v1.3 to pin A2 of the Arduino shield
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
#include "AirQuality2.h"

ATTDevice Device(&Serial1);
char httpServer[] = "api.smartliving.io";          // HTTP API Server host
char mqttServer[] = "broker.smartliving.io";       // MQTT Server Address

// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define airId 2                // Analog sensor is connected to pin A2 on grove shield
#define textId 3               // Extra asset to display labeled values
AirQuality2 airqualitysensor;

// Required for the device
void callback(int pin, String& value);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging  
  while (!Serial && millis() < 1000) ;                        // This line makes sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc
  
  Serial.println("Starting sketch");
  Serial1.begin(115200);                               // Init serial link for WiFi module
  while(!Serial1);

  while(!Device.StartWifi())
    Serial.println("Retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))   // If we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("Retrying...");
  while(!Device.Connect(httpServer))                   // Connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("Retrying");

  Device.AddAsset(airId, "Air quality sensor v1.3", "air", false, "{\"type\": \"integer\",\"minimum\":0,\"maximum\":1023}");   // Create the sensor asset for your device
  Device.AddAsset(textId, "Air quality", "label", false, "string");

  delay(1000);                                         // Give the WiFi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud (MQTT). This stops the http connection
    Serial.println("Retrying");

  pinMode(airId, INPUT);                               // Initialize the digital pin as an input.
  airqualitysensor.init(airId);
  Serial.println("Air quality sensor v1.3 is ready for use!");
}

int sensorVal = -1;                                    // Set initial value to -1
int evaluateVal = -1;

void loop() 
{
  int sensorRead = airqualitysensor.getRawData();
  if (sensorVal != sensorRead ) 
  {
    Serial.print("Air quality: ");
    Serial.print(sensorVal);
    Serial.println("(raw)");
    sensorVal = sensorRead;
    Device.Send(String(sensorVal), airId);
  }
    
  sensorRead = airqualitysensor.evaluate();
  if (evaluateVal != sensorRead ) 
  {
    String text;
    evaluateVal = sensorRead;
    if(evaluateVal == 0)
      text = "good air quality";
    else if(evaluateVal == 1)
      text = "low pollution";
    else if(evaluateVal == 2)
      text = "high pollution";
    if(evaluateVal == 3)
      text = "very high pollution";
    Serial.println(text);
    Device.Send(text, textId);
  }
  Device.Process();
  delay(2000);
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
  Serial.print("incoming data for: ");       // Display the value that arrived from the AllThingsTalk IOT developer cloud.
  Serial.print(pin);
  Serial.print(", value: ");
  Serial.println(value);
}

