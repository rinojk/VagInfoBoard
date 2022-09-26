#include <Arduino.h>
#include "DisplayManager.h"

DisplayManager::DisplayManager(){
}

DisplayManager::~DisplayManager(){

}

///
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
///

//-----------------------Display Module------------------------------//
void DisplayManager::setupDisplay()
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
  
  
  display.clearDisplay();

  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);
  display.setCursor(34, 24);
  display.println("VAG");
  display.setCursor(12, 56);
  display.println("KWP1281");
  display.display();
}

void DisplayManager::displayInfo(String text1, String text2, String text3, String mainText, bool warning)
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

void DisplayManager::renderScreen1(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData)
{

  display.clearDisplay();
  //display.drawLine(0, 33, 128, 33, WHITE);
  //display.drawLine(62, 0, 62, 64, WHITE);
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
  //display.print(String(slave_data.diagConnectionStatus) + String(slave_data.connectionStatus) + String(slave_data.isEngineWorking));

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

void DisplayManager::renderScreen2(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData)
{

  display.clearDisplay();
  //display.drawLine(0, 33, 128, 33, WHITE);
  //display.drawLine(62, 0, 62, 64, WHITE);
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
  //display.print(String(slave_data.diagConnectionStatus) + String(slave_data.connectionStatus) + String(slave_data.isEngineWorking));

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

void DisplayManager::renderScreen3(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData)
{
  display.clearDisplay();
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);
  //display.print("Engine hours; ")+String(slave_data.odomoter));
  display.setCursor(2, 36);
  display.print("Odo");
  display.setCursor(64, 36);
  display.print("Avg speed");

  display.setFont(&FreeSans12pt7b);
  display.setCursor(12, 30);
  display.print(String(infoBoardData.engineHours) + "h:" + String(infoBoardData.engineMinutes) + "m");
  display.setCursor(2, 62);
  if(slave_data.odomoter>100 && slave_data.odomoter-infoBoardData.initOdometerValue >= 0)
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

void DisplayManager::renderScreen(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData)
{
  if (infoBoardData.lastSelectedScreen == 1)
  {
    renderScreen1(slave_data, infoBoardData);
  }
  else if (infoBoardData.lastSelectedScreen == 2)
  {
    renderScreen2(slave_data, infoBoardData);
  }
  else if (infoBoardData.lastSelectedScreen == 3)
  {
    renderScreen3(slave_data, infoBoardData);
  }
}


