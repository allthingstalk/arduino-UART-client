/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield
  - Connect USB cable to your computer
  - Connect a Grove Push button to pin D2 of the Arduino shield
  - Grove UART wifi to pin UART (D0,D1)
2. Add 'ATT_IOT_UART' library to your Arduino Environment
     More info can be found at http://arduino.cc/en/Guide/Libraries
3. Fill in the missing strings (deviceId, clientId and clientKey) in the keys.h file
4. Optionally, change sensor names, labels as appropiate
5. Upload the sketch

Note: for use of extra actuators, extend the callback function at the end of the sketch

*/

#include "ATT_IOT_UART.h"                       // AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                // Required to have support for signed/unsigned long type.
#include "keys.h"                               // Keep all your personal account information in a seperate file

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define PIN numbers for assets
#define DigitalSensor 2
#define Led 3                                   // Feedback from the cloud to show that the connection is working

//required for the device
void callback(int pin, String& value);


void setup() 
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc
  Serial.println("Starting sketch");
  Serial1.begin(115200);                               // Init serial link for wifi module
  while(!Serial1);
  
  while(!Device.StartWifi())
    Serial.println("retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))       // If we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.Connect(httpServer))                       // Connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("retrying");
    
  Device.AddAsset(DigitalSensor, "push button", "push button", false, "boolean");   // Create the Sensor asset for your device
  Device.AddAsset(Led, "led", "activate me to show that the connection is working", true, "boolean"); 
  
  delay(1000);                                         // Give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
    Serial.println("retrying");
    
  pinMode(DigitalSensor, INPUT);                       // Initialize the digital pin as an input.          
  pinMode(Led, OUTPUT);                                // Initialize the digital pin as an input      

}

bool sensorVal = false;

void loop() 
{
  bool sensorRead = digitalRead(DigitalSensor);             // Read status Digital Sensor
  if (sensorVal != sensorRead)                              // Verify if value has changed
  {
  	if(sensorRead == true){
	    Serial.println("resetting wifi");
  	    while(!Device.StartWifi(true))
  			Serial.println("retrying...");
  		while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))       // If we can't succeed to initialize and set the device credentials, there is no point to continue
  			Serial.println("retrying...");
		delay(100);
  		while(!Device.Subscribe(mqttServer, callback))           // Make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
  			Serial.println("retrying");
  	}
    sensorVal = sensorRead;
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
  if(pin == Led)
  {
    Serial.print("Incoming data for: ");               // Display the value that arrived from the cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.println(value);

    if(value == "true")
      digitalWrite(pin, HIGH);
    else    
      digitalWrite(pin, LOW);
    Device.Send(value, pin);                          // Send the value back for confirmation
  }
}

