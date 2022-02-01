#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans18pt7b.h>
#include "i2cSimpleTransfer.h"

#pragma region Display
///
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
///

//-----------------------Display Module------------------------------//
void setupDisplay()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ;
  }
  //display.setFont(&FreeMono9pt7b);
  display.setTextSize(1);
  display.setRotation(2);
  display.setTextColor(WHITE);
  display.cp437(true);
  displayInfo("HELLO WRLD!");
}

void displayInfo(String text)
{
  display.clearDisplay();

  display.setFont();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(text);
  display.display();
}

void displayInfo(String text1, String text2, String text3, String mainText, bool warning)
{
  display.clearDisplay();

  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(text1);
  display.println(text2);

  display.setTextSize(2);
  display.println(text3);

  display.setFont(&FreeSans18pt7b);
  display.setTextSize(1);
  int shift = 24;
  display.setCursor(69, shift);
  display.println(mainText);

  if (warning)
  {
    display.setFont();
    display.setTextSize(4);
    display.setCursor(54, 0);
    display.print("!");
  }
  display.display();
}
#pragma endregion

struct DIAG_DATA_STRUCTURE
{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int16_t blinks;
  int16_t pause;
  int16_t connectionStatus = 0;
  int16_t oilTemp = 0;
  int16_t coolantTemp = 0;
  int16_t MAF = 0;
  int16_t misfireCounter = 0;
  int16_t batteryVoltage = 0;
};

//give a name to the group of data
DIAG_DATA_STRUCTURE mydata;

#define i2c_sensor_slave 17

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
  int16_t sensor; // use specific declarations to ensure communication between 16bit and 32bit controllers
  int16_t connectionStatus = 0;
  int16_t oilTemp = 0;
  int16_t coolantTemp = 0;
  int16_t MAF = 0;
  int16_t misfireCounter = 0;
  int16_t batteryVoltage = 0;
};

struct SLAVE_CONFIG
{
  uint8_t val; // use specific declarations to ensure communication between 16bit and 32bit controllers
};

SLAVE_DATA slave_data;
SLAVE_CONFIG slave_config;

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  setupDisplay();
  Serial.begin(115200); // start serial for output
}

void loop()
{
  Wire.requestFrom(i2c_sensor_slave, sizeof(slave_data)); // request data from the Slave device the size of our struct

  if (Wire.available() == sizeof(slave_data))
  {
    i2cSimpleRead(slave_data);
  }
  showData();
  displayInfo(String(String(slave_data.sensor)+" "+String(slave_data.oilTemp)), String(slave_data.coolantTemp), String(slave_data.misfireCounter), String(slave_data.batteryVoltage),false);
  if (slave_data.sensor == 100)
  {
    slave_config.val = 100;
    Wire.beginTransmission(i2c_sensor_slave);
    i2cSimpleWrite(slave_config);
    Wire.endTransmission();
  }

  delay(500);
}

void showData()
{
  Serial.println("DATA RECEIVED:");
  Serial.println(mydata.connectionStatus);
  Serial.println(mydata.oilTemp);
  Serial.println(mydata.coolantTemp);
  Serial.println(mydata.MAF / 100.0);
  Serial.println(mydata.misfireCounter);
  Serial.println(mydata.batteryVoltage / 100.0);
}

void convertByteArrayToStructure()
{
}