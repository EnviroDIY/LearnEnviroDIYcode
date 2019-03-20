/********************************************************************/
// First we include the libraries
#include <OneWire.h>
#include <DallasTemperature.h>
/********************************************************************/
// Data wire is plugged into pin 7 on the Arduino
#define ONE_WIRE_BUS 7 // For EnviroDIY Mayfly, I am using digital pin 7.
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
/********************************************************************/


// define conversion factor for miliseconds to seconds (multiply by 1000)
int time_conversion_factor = 1000;
int delaytime = 5 * time_conversion_factor;

const int8_t switchedPower = 22;  // Pin to switch power on and off (-1 if unconnected)


void setup()
{
 // Turn on switched power
 pinMode(switchedPower, OUTPUT);
 digitalWrite(switchedPower, HIGH);
 delay(delaytime);

 // start serial port
 Serial.begin(57600);
 Serial.println("DS18B20 One Wire Temperature Demo");
 // Start up the library
 sensors.begin();
}
void loop()
{
 // call sensors.requestTemperatures() to issue a global temperature
 // request to all devices on the bus
/********************************************************************/
 Serial.print(" Requesting temperatures...");
 sensors.requestTemperatures(); // Send the command to get temperature readings
 Serial.println("DONE");
/********************************************************************/
 Serial.print("Temperature is: ");
 Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"?
   // You can have more than one DS18B20 on the same bus.
   // 0 refers to the first IC on the wire
   delay(delaytime);
}
