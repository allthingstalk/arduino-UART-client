/*

Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove
UART WiFi module based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk
IoT developer cloud.
The Grove UART WiFi module has firmware installed which includes the ATT_IOT library. It
communicates through Serial1 of the Genuino 101 board.

Version 1.0 dd 26/12/2015

This sketch is an example sketch to deploy the Grove GPS module (113020003) to the
AllThingsTalk IoT developer cloud.


### Instructions
1. Setup the Arduino hardware
  - Use an Arduino Genuino 101 IoT board
  - Connect the Arduino Grove shield, make sure the switch is set to 3.3V
  - Connect USB cable to your computer
  - Connect a Grove GPS module to pin D2 of the Arduino shield
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
#include <SoftwareSerial.h>

ATTDevice Device(&Serial1);
char httpServer[] = "api.smartliving.io";          // HTTP API Server host
char mqttServer[] = "broker.smartliving.io";       // MQTT Server Address

// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define gpsId 2                // Digital sensor is connected to pin D2 on grove shield

SoftwareSerial SoftSerial(2, 3);
unsigned char buffer[64];                   // Buffer array for data receive over serial port
int count=0;

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

  Device.AddAsset(gpsId, "GPS module", "GPS", false, "{\"type\": \"object\",\"properties\": {\"lat\": {\"type\": \"number\", \"unit\": \"°\"},\"lng\": {\"type\": \"number\", \"unit\": \"°\"},\"alt\": {\"type\": \"number\", \"unit\": \"°\"},\"time\": {\"type\": \"integer\", \"unit\": \"epoc time\"}},\"required\": [\"lat\",\"lng\",\"alt\",\"time\" ]}");   // Create the sensor asset for your device

  delay(1000);                                         // Give the WiFi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))       // Make sure that we can receive message from the AllThingsTalk IOT developer cloud (MQTT). This stops the http connection
    Serial.println("Retrying");

  pinMode(gpsId, INPUT);                               // Initialize the digital pin as an input.
  Serial.println("GPS module is ready for use!");
}


float longitude;
float latitude;
float altitude;
float timestamp;

void loop() 
{
  if(readCoordinates() == true) SendValue();
  Device.Process();
  delay(5000);
}

void SendValue()
{
    Serial.print("sending gps data");
	  String data;
	  data = "{\"lat\":" + String(latitude, 3) + ",\"lng\":" + String(longitude, 3) + ",\"alt\":" + String(altitude, 3) + ",\"time\":" + String((int)timestamp) + "}";
    Device.Send(data, gpsId);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    Serial.print("incoming data for: ");         // Display the value that arrived from the AllThingsTalk IOT developer cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.println(value);
}

bool readCoordinates()
{
  bool foundGPGGA = false;                          // Sensor can return multiple types of data, need to capture lines that start with $GPGGA
    if (SoftSerial.available())                     // If date is coming from software serial port ==> data is coming from SoftSerial shield
    {
        while(SoftSerial.available())               // Reading data into char array
        {
            buffer[count++]=SoftSerial.read();      // Writing data into array
            if(count == 64)break;
        }
        //use the following line to see the raw data coming from the gps
        //Serial.println((char*)buffer);
        foundGPGGA = count > 60 && ExtractValues();  // If we have less then 60 characters, then we have bogus input, so don't try to parse it or process the values
        clearBufferArray();                          // Call clearBufferArray function to clear the stored data from the array
        count = 0;                                   // Set counter of while loop to zero
    }
    return foundGPGGA;
}

bool ExtractValues()
{
  unsigned char start = count;
  while(buffer[start] != '$')        // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
  {
    if(start == 0) break;            // It's an unsigned char, so we can't check on <= 0
    start--;
  }
  start++;                           // Remove the '$', don't need to compare with that.
  if(start + 4 < 64 && buffer[start] == 'G' && buffer[start+1] == 'P' && buffer[start+2] == 'G' && buffer[start+3] == 'G' && buffer[start+4] == 'A')      //we found the correct line, so extract the values.
  {
    start+=6;
    timestamp = ExtractValue(start);
    latitude = ConvertDegrees(ExtractValue(start) / 100);
    start = Skip(start);    
    longitude = ConvertDegrees(ExtractValue(start)  / 100);
    start = Skip(start);
    start = Skip(start);
    start = Skip(start);
    start = Skip(start);
    altitude = ExtractValue(start);
    return true;
  }
  else
    return false;
}

float ExtractValue(unsigned char& start)
{
  unsigned char end = start + 1;
  while(end < count && buffer[end] != ',')      // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
    end++;
  buffer[end] = 0;                              // End the string, so we can create a string object from the sub string -> easy to convert to float.
  float result = 0.0;
  if(end != start + 1)                          // If we only found a ',', then there is no value.
    result = String((const char*)(buffer + start)).toFloat();
  start = end + 1;
  return result;
}

float ConvertDegrees(float input)
{
  float fractional = input - (int)input;
  return (int)input + (fractional / 60.0) * 100.0;
}

unsigned char Skip(unsigned char start)
{
  unsigned char end = start + 1;
  while(end < count && buffer[end] != ',')      // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
    end++;
  return end+1;
}

void clearBufferArray()                         // Function to clear buffer array
{
  for (int i=0; i<count; i++)
  {
    buffer[i] = NULL;                           // Clear all index of array with command NULL
  }
}

