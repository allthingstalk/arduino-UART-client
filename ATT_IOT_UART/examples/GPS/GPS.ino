/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the Grove - GPS (113020003) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield
	- Connect USB cable to your computer
    - Connect a Grove GPS to PIN D2 of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Fill in the missing strings (deviceId, clientId, clientKey) in the keys.h file. 
  4. Optionally, change sensor names, labels as appropriate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch
*/


#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           //keep all your personal account information in a seperate file
#include <SoftwareSerial.h>

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define the assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id.  
// For other sensors (I2C and UART), you can select any other (unique) number as id for the asset.
#define GPSId 2             
SoftwareSerial SoftSerial(2, 3);
unsigned char buffer[64];                   // buffer array for data receive over serial port
int count=0;  

//required for the device
void callback(int pin, String& value);


void setup() 
{
  SoftSerial.begin(9600); 
  Serial.begin(57600);                                         // init serial link for debugging
  
  while (!Serial) ;                                            // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting sketch");
  Serial1.begin(115200);                                       //init serial link for wifi module
  while(!Serial1);
  
  while(!Device.StartWifi())
    Serial.println("retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))           //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.Connect(httpServer))                           // connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("retrying");
    
  Device.AddAsset(GPSId, "GPS", "GPS location", false, "{\"type\": \"object\",\"properties\": {\"latitude\": {\"type\": \"number\", \"unit\": \"°\"},\"longitude\": {\"type\": \"number\", \"unit\": \"°\"},\"altitude\": {\"type\": \"number\", \"unit\": \"°\"},\"time\": {\"type\": \"number\", \"unit\": \"epoc time\"}},\"required\": [\"latitude\",\"longitude\",\"longitude\",\"time\" ]}");   // Create the Sensor asset for your device
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
    Serial.println("retrying");
	
  Serial.println("GPS is spinning up!");	
}


float longitude;
float latitude;
float altitude;
float timestamp;

void loop() 
{
  if(readCoordinates() == true) SendValue();
  Device.Process();
  delay(3000);
}

void SendValue()
{
    Serial.print("sending gps data");
	  String data;
	  data = "{\"latitude\": " + String(latitude) + ", \"longitude\"" + String(longitude) + ", \"altitude\"" + String(altitude) + ", \"time\"" + String(timestamp) + "}";
    Device.Send(data, GPSId);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    Serial.print("incoming data for: ");               //display the value that arrived from the AllThingsTalk IOT developer cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.print(value);
}

bool readCoordinates()
{
  bool foundGPGGA = false;                          // sensor can return multiple types of data, need to capture lines that start with $GPGGA
    if (SoftSerial.available())                     // if date is coming from software serial port ==> data is coming from SoftSerial shield
    {
        while(SoftSerial.available())               // reading data into char array
        {
            buffer[count++]=SoftSerial.read();      // writing data into array
            if(count == 64)break;
        }
        //Serial.println(count); 
        Serial.println((char*)buffer);
        foundGPGGA = count > 60 && ExtractValues();  // if we have less then 60 characters, then we have bogus input, so don't try to parse it or process the values
        clearBufferArray();                          // call clearBufferArray function to clear the stored data from the array
        count = 0;                                   // set counter of while loop to zero
    }
    return foundGPGGA;
}

bool ExtractValues()
{
    unsigned char start = count;
    while(buffer[start] != '$')     // find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
    {
        if(start == 0) break;                       // it's unsigned char, so we can't check on <= 0
			start--;
    }
    start++;                                        // remove the '$', don't need to compare with that.
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
    while(end < count && buffer[end] != ',')      // find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
        end++;
    buffer[end] = 0;                              // end the string, so we can create a string object from the sub string -> easy to convert to float.
    float result = 0.0;
    if(end != start + 1)                          // if we only found a ',', then there is no value.
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
    while(end < count && buffer[end] != ',')        // find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
        end++;
    return end+1;
}

void clearBufferArray()                     // function to clear buffer array
{
    for (int i=0; i<count;i++)
    { buffer[i]=NULL;}                      // clear all index of array with command NULL
}

