#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>
#include "i2cSimpleTransfer.h"
#include <EEPROM.h>

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
  // TO BE RENAMED TO diagConnectionStatus
  //  0: not connected
  //  1: connected to ECU
  //  2: connected to DASH
  int16_t diagConnectionStatus = 0; // use specific declarations to ensure communication between 16bit and 32bit controllers
  // TO BE RENAMED TO bridgeConnectionStatus
  //  0: not connected
  //  1: connected
  int16_t connectionStatus = 0;
  int16_t rpm = 0;
  int16_t vehicleSpeed = 0;
  int16_t oilTemp = 0;
  int16_t coolantTemp = 0;
  int16_t MAF = 0;
  int16_t misfireCounter = 0;
  int16_t intakeAirTemp = 0;
  int16_t batteryVoltage = 0;
  int16_t isEngineWorking = 0;
  int16_t fuelLevel = 0;
  unsigned long odomoter = 0;
};

struct SLAVE_CONFIG
{
  uint8_t val; // use specific declarations to ensure communication between 16bit and 32bit controllers
};

struct BoardInfoSystemData
{
  uint16_t engineHours = 1;
  uint8_t engineMinutes = 1;
  uint8_t lastSelectedScreen = 2;
  unsigned long initOdometerValue = 0;
};

SLAVE_DATA slave_data;
SLAVE_CONFIG slave_config;
BoardInfoSystemData infoBoardData;

#pragma region Display
///
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
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
  // display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);
  //display.setRotation(2);
  display.setTextColor(WHITE);
  display.cp437(true);
  displayInfo("VAG:KWP1281");
}

void displayInfo(String text)
{
  display.clearDisplay();

  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);
  display.setCursor(0, 24);
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

  display.setFont(&FreeSans12pt7b);
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

void renderScreen()
{
  if (infoBoardData.lastSelectedScreen == 1)
  {
    renderScreen1();
  }
  else if (infoBoardData.lastSelectedScreen == 2)
  {
    renderScreen2();
  }
  else if (infoBoardData.lastSelectedScreen == 3)
  {
    renderScreen3();
  }
}

void renderScreen1()
{

  display.clearDisplay();
  display.drawLine(0, 33, 128, 33, WHITE);
  display.drawLine(62, 0, 62, 64, WHITE);
  display.setFont();
  display.setTextSize(1);
  display.setCursor(8, 0);
  display.print("Oil temp"); //+String(slave_data.intakeAirTemp));
  display.setCursor(68, 0);
  display.print("Cool temp");
  display.setCursor(4, 36);
  display.print("Int. temp");
  display.setCursor(72, 36);
  display.print("Fuel lvl");
  // display.setTextSize(2);
  display.setCursor(46, 56);
  display.print(String(slave_data.diagConnectionStatus) + String(slave_data.connectionStatus) + String(slave_data.isEngineWorking));

  if (slave_data.connectionStatus == 1)
  {
    display.setFont(&FreeSans12pt7b);
    display.setCursor(12, 30);
    display.print(slave_data.oilTemp);
    display.setCursor(76, 30);
    display.print(slave_data.coolantTemp);
    display.setCursor(12, 62);
    display.print(slave_data.intakeAirTemp);
    display.setCursor(64, 62);
    display.print(slave_data.fuelLevel);
  }
  else if (slave_data.connectionStatus == 0)
  {
    display.setFont(&FreeSans12pt7b);
    display.setCursor(12, 30);
    display.print("---");
    display.setCursor(76, 30);
    display.print("---");
    display.setCursor(12, 62);
    display.print("---");
    display.setCursor(74, 62);
    display.print("--.--");
  }
  // display.setTextSize(1);
  // int shift = 24;
  // display.setCursor(59, shift);
  // display.println("! 123");//+String(slave_data.coolantTemp));
  display.display();
}

void renderScreen2()
{

  display.clearDisplay();
  display.drawLine(0, 33, 128, 33, WHITE);
  display.drawLine(62, 0, 62, 64, WHITE);
  display.setFont();
  display.setTextSize(1);
  display.setCursor(8, 0);
  display.print("Oil temp"); //+String(slave_data.intakeAirTemp));
  display.setCursor(68, 0);
  display.print("Cool temp");
  display.setCursor(4, 36);
  display.print("Int. temp");
  display.setCursor(72, 36);
  display.print("Battery");
  // display.setTextSize(2);
  display.setCursor(46, 56);
  display.print(String(slave_data.diagConnectionStatus) + String(slave_data.connectionStatus) + String(slave_data.isEngineWorking));

  if (slave_data.connectionStatus == 1)
  {
    display.setFont(&FreeSans12pt7b);
    display.setCursor(12, 30);
    display.print(slave_data.oilTemp);
    display.setCursor(76, 30);
    display.print(slave_data.coolantTemp);
    display.setCursor(12, 62);
    display.print(slave_data.intakeAirTemp);
    display.setCursor(64, 62);
    display.print(slave_data.batteryVoltage / 100.0);
  }
  else if (slave_data.connectionStatus == 0)
  {
    display.setFont(&FreeSans12pt7b);
    display.setCursor(12, 30);
    display.print("---");
    display.setCursor(76, 30);
    display.print("---");
    display.setCursor(12, 62);
    display.print("---");
    display.setCursor(74, 62);
    display.print("--.--");
  }
  // display.setTextSize(1);
  // int shift = 24;
  // display.setCursor(59, shift);
  // display.println("! 123");//+String(slave_data.coolantTemp));
  display.display();
}

void renderScreen3()
{
  display.clearDisplay();
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Engine hours; "+String(slave_data.odomoter));
  display.setCursor(2, 36);
  display.print("Odo");
  display.setCursor(64, 36);
  display.print("Avg speed");

  display.setFont(&FreeSans12pt7b);
  display.setCursor(12, 30);
  display.print(String(infoBoardData.engineHours) + "h:" + String(infoBoardData.engineMinutes) + "m");
  display.setCursor(2, 62);
  if(slave_data.odomoter-infoBoardData.initOdometerValue >= 0)
    display.print(String(slave_data.odomoter-infoBoardData.initOdometerValue));
  else
    display.print("---");
  display.setCursor(64, 62);
  float avgSpeed = 0.0;
  if((infoBoardData.engineHours+(infoBoardData.engineMinutes/60.0))>0.0 && (slave_data.odomoter-infoBoardData.initOdometerValue)>=0)
    avgSpeed = (slave_data.odomoter-infoBoardData.initOdometerValue)/(infoBoardData.engineHours+(infoBoardData.engineMinutes/60.0));
  display.print(String(avgSpeed));

  display.display();
}
#pragma endregion

#pragma region I2C Master managing
#define i2c_sensor_slave 17

void manageWire()
{
  Wire.requestFrom(i2c_sensor_slave, sizeof(slave_data)); // request data from the Slave device the size of our struct

  if (Wire.available() == sizeof(slave_data))
  {
    i2cSimpleRead(slave_data);
    // displayInfo(String(String(millis()/1000) + " " + String(slave_data.intakeAirTemp)), String(slave_data.coolantTemp), String(slave_data.misfireCounter), String(slave_data.oilTemp), false);
    renderScreen();
  }
  // printDataToSerial();
  //  displayInfo(String(String(millis()/1000) + " " + String(slave_data.batteryVoltage/100)), String(slave_data.coolantTemp), String(slave_data.misfireCounter), String(slave_data.batteryVoltage), false);

  slave_config.val = 100;
  Wire.beginTransmission(i2c_sensor_slave);
  i2cSimpleWrite(slave_config);
  Wire.endTransmission();
}

#pragma endregion

#pragma region EEPROM
byte eepromAddress = 0;
void setupEEPROMData()
{
  EEPROM.begin(255);
  EEPROM.get(0, infoBoardData);
  Serial.println(infoBoardData.initOdometerValue);
}

void updateData()
{
  Serial.println("eeprom write");
  EEPROM.put(0, infoBoardData);
  EEPROM.commit();
}

unsigned long lastWrittenEngineHrsMillis = 0;
void handleEngineHrs(){
  if(slave_data.isEngineWorking && (millis() - lastWrittenEngineHrsMillis)>60000){
    if(infoBoardData.engineMinutes>=59){
      infoBoardData.engineHours += 1;
      infoBoardData.engineMinutes = 0;
    }
    else{
      infoBoardData.engineMinutes++;
    }
    lastWrittenEngineHrsMillis = millis();
    updateData();
  }
}
#pragma endregion

#pragma region Click handlers
bool touchActivated = false;
unsigned long clickStartTime = 0;

void handleClick()
{
  bool isClicked = (touchRead(15) < 30);
  // Start touch
  if (isClicked && !touchActivated)
  {
    clickStartTime = millis();
    touchActivated = true;
    Serial.println("Btn is pressed");
  }

  // Stop touch
  if (!isClicked && touchActivated)
  {
    Serial.println("Btn stopped to be pressed");
    if ((millis() - clickStartTime) > 100 && (millis() - clickStartTime) < 1000)
    {
      handleShortClick();
    }
    else if((millis() - clickStartTime) > 1000)
    {
      handleLongClick();
    }
    touchActivated = false;
    clickStartTime = 0;
  }
}

void handleShortClick()
{
  Serial.println("SHORT CLICK");
  switchDisplay();
}

void handleLongClick()
{
  Serial.println("LONG CLICK");

  infoBoardData.engineHours=0;
  infoBoardData.engineMinutes=0;
  infoBoardData.initOdometerValue=slave_data.odomoter;
  updateData();
}
#pragma endregion

void printDataToSerial()
{
  Serial.println("DATA RECEIVED:");
  Serial.print("Status ");
  Serial.print(slave_data.connectionStatus);
  Serial.println();
  Serial.print("Oil temp ");
  Serial.print(slave_data.oilTemp);
  Serial.println();
  Serial.print("Coolant temp ");
  Serial.print(slave_data.coolantTemp);
  Serial.println();
  Serial.print("MAF ");
  Serial.print(slave_data.MAF);
  Serial.println();
  Serial.print("Misfires ");
  Serial.print(slave_data.misfireCounter);
  Serial.println();
  Serial.print("Intake temp ");
  Serial.print(slave_data.intakeAirTemp);
  Serial.print("Is engine working ");
  Serial.print(slave_data.isEngineWorking);
  Serial.print("Battery V ");
  Serial.print(slave_data.batteryVoltage / 100.0);
  Serial.println();
}

void switchDisplay()
{
  if (infoBoardData.lastSelectedScreen > 3)
  {
    infoBoardData.lastSelectedScreen = 1;
  }
  else
  {
    infoBoardData.lastSelectedScreen++;
  }

  renderScreen();
}

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  setupDisplay();
  Serial.begin(9600); // start serial for output
  //EEPROM.put(0, infoBoardData);
  setupEEPROMData();
}

unsigned long lastLoopTime = 0;

void loop()
{
  if (millis() - lastLoopTime > 500)
  {
    manageWire();
    lastLoopTime = millis();
  }
  // delay(500);
  handleClick();

  handleEngineHrs();
}