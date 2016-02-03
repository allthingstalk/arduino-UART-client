/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the Grove - digital compass (101020034) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield
    - Connect USB cable to your computer
    - Connect a Grove digital compass to I2c of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Fill in the missing strings (deviceId, clientId, clientKey) in the keys.h file. 
  4. Optionally, change sensor names, labels as appropriate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch
*/

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                               //keep all your personal account information in a seperate file
#include <Wire.h>                               // Reference the I2C Library
#include <HMC5883L.h>                           // Reference the HMC5883L Compass Library   

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define the assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id.  
// For other sensors (I2C and UART), you can select any other (unique) number as id for the asset.
#define compassId 2
MMA7660 compass; 
MagnetometerScaled valueOffset;
int error = 0;                                                  // Record any errors that may occur in the compass.

#define MAGNETIC_DECLINATION 0.0145                             // comes from the following calculation: delcenation in Belgium = 0degrees, 50 minutes = 0.8333 degrees.  to radian = 0.8333 / 360 * 2 * pi

//required for the device
void callback(int pin, String& value);


void setup()
{
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
    
  Device.AddAsset(compassId, "accelerometer", "accelerometer", false, "{\"type\": \"object\",\"properties\": {\"x\": {\"type\": \"number\"},\"y\": {\"type\": \"number\"},\"z\": {\"type\": \"number\"}},\"required\": [\"x\",\"y\",\"z\"]}");   // Create the Sensor asset for your device
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
    Serial.println("retrying");
    
  Wire.begin();                                                 // Start the I2C interface.
  Serial.println("Constructing new HMC5883L");
    
  Serial.println("Setting scale to +/- 1.3 Ga");
  error = compass.setScale(1.3);                                // Set the scale of the compass. Magic number, don't know the origin...
  if(error != 0)                                                // If there is an error, print it out.
    Serial.println(compass.getErrorText(error));
  
  Serial.println("Setting measurement mode to continous.");
  error = compass.setMeasurementMode(MEASUREMENT_CONTINUOUS);   // Set the measurement mode to Continuous
  if(error != 0)                                                // If there is an error, print it out.
    Serial.println(compass.getErrorText(error));
    
  compassCalibrate();
  Serial.println("compass is ready!");  
}

// calibrate offset of x, y and z
void compassCalibrate(void)
{
  //Serial << ">>>> calibrate the compass\n";
  Serial.println("calibrate the compass");
  MagnetometerScaled valueMax = {0, 0, 0};
  MagnetometerScaled valueMin = {0, 0, 0};

  Serial.println("please rotate the compass");
  int xcount = 0;
  int ycount = 0;
  int zcount = 0;
  boolean xZero = false;
  boolean yZero = false;
  boolean zZero = false;
  MagnetometerScaled value;
  while (xcount < 3 || ycount < 3 || zcount < 3) {
    value = compass.readScaledAxis();
    if ((fabs(value.XAxis) > 600) || (fabs(value.YAxis) > 600) || (fabs(value.ZAxis) > 600)) {
      continue;
    }
    
    if (valueMin.XAxis > value.XAxis) {
      valueMin.XAxis = value.XAxis;
    } else if (valueMax.XAxis < value.XAxis) {
      valueMax.XAxis = value.XAxis;
    }
    
    if (valueMin.YAxis > value.YAxis) {
      valueMin.YAxis = value.YAxis;
    } else if (valueMax.YAxis < value.YAxis) {
      valueMax.YAxis = value.YAxis;
    }
    
    if (valueMin.ZAxis > value.ZAxis) {
      valueMin.ZAxis = value.ZAxis;
    } else if (valueMax.ZAxis < value.ZAxis) {
      valueMax.ZAxis = value.ZAxis;
    }
    
    
    if (xZero) {
      if (fabs(value.XAxis) > 50) {
        xZero = false;
        xcount++;
      }
    } else {
      if (fabs(value.XAxis) < 40) {
        xZero = true;
      }
    }
    
    if (yZero) {
      if (fabs(value.YAxis) > 50) {
        yZero = false;
        ycount++;
      }
    } else {
      if (fabs(value.YAxis) < 40) {
        yZero = true;
      }
    }
    
    if (zZero) {
      if (fabs(value.ZAxis) > 50) {
        zZero = false;
        zcount++;
      }
    } else {
      if (fabs(value.ZAxis) < 40) {
        zZero = true;
      }
    }
    
    delay(30);
  }
  valueOffset.XAxis = (valueMax.XAxis + valueMin.XAxis) / 2;
  valueOffset.YAxis = (valueMax.YAxis + valueMin.YAxis) / 2;
  valueOffset.ZAxis = (valueMax.ZAxis + valueMin.ZAxis) / 2;
#if 0 
  Serial << "max: " << valueMax.XAxis << '\t' << valueMax.YAxis << '\t' << valueMax.ZAxis << endl;
  Serial << "min: " << valueMin.XAxis << '\t' << valueMin.YAxis << '\t' << valueMin.ZAxis << endl;
  Serial << "offset: " << valueOffset.XAxis << '\t' << valueOffset.YAxis << '\t' << valueOffset.ZAxis << endl;
  
  Serial << "<<<<" << endl;
#endif  
  Serial.print("max: ");
  Serial.print(valueMax.XAxis);
  Serial.print(valueMax.YAxis);
  Serial.println(valueMax.ZAxis);
  Serial.print("min: ");
  Serial.print(valueMin.XAxis);
  Serial.print(valueMin.YAxis);
  Serial.println(valueMin.ZAxis);
  Serial.print("offset: ");
  Serial.print(valueOffset.XAxis);
  Serial.print(valueOffset.YAxis);
  Serial.println(valueOffset.ZAxis);
}

void loop()
{
  // Retrive the raw values from the compass (not scaled).
  MagnetometerRaw raw = compass.readRawAxis();
  // Retrived the scaled values from the compass (scaled to the configured scale).
  MagnetometerScaled scaled = compass.readScaledAxis();
  
  scaled.XAxis -= valueOffset.XAxis;
  scaled.YAxis -= valueOffset.YAxis;
  scaled.ZAxis -= valueOffset.ZAxis;
  
  // Values are accessed like so:
  int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)

  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float yxHeading = atan2(scaled.YAxis, scaled.XAxis);
  float zxHeading = atan2(scaled.ZAxis, scaled.XAxis);
  
  float heading = yxHeading;
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  float declinationAngle = -0.0457;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  
  float yxHeadingDegrees = yxHeading * 180 / M_PI;
  float zxHeadingDegrees = zxHeading * 180 / M_PI;

  Serial.print(scaled.XAxis);
  Serial.print(scaled.YAxis);
  Serial.println(scaled.ZAxis);
  
  Serial.print("arctan y/x: ");
  Serial.print(yxHeadingDegrees);
  Serial.print("arctan z/x: ");  
  Serial.print(zxHeadingDegrees);
  SendValue(scaled);
  Device.Process();
  // Normally we would delay the application by 66ms to allow the loop
  // to run at 15Hz (default bandwidth for the HMC5883L).
  // However since we have a long serial out (104ms at 9600) we will let
  // it run at its natural speed.
  delay(1000);
}

void SendValue(MagnetometerScaled scaled)
{
    Serial.print("sending compass data");
    String data;
    data = "{\"x\": " + String(scaled.XAxis) + ", \"y\"" + String(scaled.YAxis) + ", \"z\"" + String(scaled.ZAxis) + "}"
    Device.Send(data, compassId);
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    Serial.print("incoming data for: ");               //display the value that arrived from the AllThingsTalk IOT developer cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.print(value);
    Device.Send(value, pin);                            //send the value back for confirmation   
}

