#include <Wire.h>
#include "i2cSimpleTransfer.h"
#include <EEPROM.h>
#include "Structs.h"
#include "DisplayManager.h"
#include "EEPROMmanager.h"

SLAVE_DATA slave_data;
SLAVE_CONFIG slave_config;
BoardInfoSystemData infoBoardData;

DisplayManager displayManager;
EEPROMmanager eepromManager;

#pragma region I2C Master managing
#define i2c_sensor_slave 17

void manageWire()
{
  Wire.requestFrom(i2c_sensor_slave, sizeof(slave_data)); // request data from the Slave device the size of our struct

  if (Wire.available() == sizeof(slave_data))
  {
    i2cSimpleRead(slave_data);
    // displayInfo(String(String(millis()/1000) + " " + String(slave_data.intakeAirTemp)), String(slave_data.coolantTemp), String(slave_data.misfireCounter), String(slave_data.oilTemp), false);
    displayManager.renderScreen(slave_data, infoBoardData);
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
    eepromManager.updateData(infoBoardData);
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
    else if((millis() - clickStartTime) > 5000)
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
  eepromManager.updateData(infoBoardData);
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

  displayManager.renderScreen(slave_data, infoBoardData);
}

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  displayManager.setupDisplay();
  Serial.begin(9600); // start serial for output
  //EEPROM.put(0, infoBoardData);
  eepromManager.setupEEPROMData(infoBoardData);
}

unsigned long lastLoopTime = 0;

void loop()
{
  if (millis() - lastLoopTime > 250)
  {
    manageWire();
    lastLoopTime = millis();
  }
  // delay(500);
  handleClick();

  handleEngineHrs();
}