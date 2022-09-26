#ifndef Structs_h
#define Structs_h

#include <Arduino.h>

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
  uint8_t engineMinutes = 2;
  uint8_t lastSelectedScreen = 2;
  unsigned long initOdometerValue = 0;
};
#endif