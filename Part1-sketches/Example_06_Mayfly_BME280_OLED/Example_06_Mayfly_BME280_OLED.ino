/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface. The device's I2C address is either 0x76 or 0x77.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SDL_Arduino_SSD1306.h>    // Modification of Adafruit_SSD1306 for ESP8266 compatibility
#include <AMAdafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <SPI.h>            // For SPI comm (needed for not getting compile error)


// #define BME_SCK 13
// #define BME_MISO 12
// #define BME_MOSI 11
// #define BME_CS 10

uint8_t BMEi2c_addr = 0x76;  // Address is 0x77 (Adafruit default) or 0x76 (Grove default)
const int8_t I2CPower = 22;  // Pin to switch power on and off (-1 if unconnected)

// Create an instance of the OLED display
SDL_Arduino_SSD1306 display(4); // FOR I2C


#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;

void setup() {
    Serial.begin(115200);
    Serial.println(F("BME280 test"));

    pinMode(5, INPUT);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false);  // initialize with the I2C addr 0x3C (for the 128x64)
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Mayfly");
    display.println("BME280 demo");
    display.display();


    // Turn on switched power
    pinMode(I2CPower, OUTPUT);
    digitalWrite(I2CPower, HIGH);

    bool status;

    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    status = bme.begin(BMEi2c_addr);
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    Serial.println("-- Timing Test --");
    delayTime = 1100;

    Serial.println();

    // Print table headers
    Serial.println("  Time,  Temp, Humid,   Press,   Alt");
    Serial.println("    ms,    *C,     %,      Pa,     m");

}


void loop() {

    for (int i=0; i <= 30; i++)
    {
        Serial.print("  ");
        Serial.print(millis());
        Serial.print(", ");
        Serial.print(bme.readTemperature());
        Serial.print(", ");
        Serial.print(bme.readHumidity());
        Serial.print(", ");
        Serial.print(bme.readPressure());
        Serial.print(", ");
        Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
        Serial.println();
        delay(delayTime);
    }



    for (int i=0; i <= 30; i++)
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print("T: "); display.print(bme.readTemperature()); display.println(" C");
      display.print("H: "); display.print(bme.readHumidity()); display.println(" %");
      display.print("P: "); display.print(bme.readPressure()); display.println(" Pa");
      display.display();

      delay(delayTime);
  }


    delay(100000);
}
