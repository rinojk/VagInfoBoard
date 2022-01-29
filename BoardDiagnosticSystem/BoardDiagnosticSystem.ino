#include <EasyTransferI2C.h>
#include <Wire.h>
#include "KWP.h"

#pragma region Timers
unsigned long loopTimer = 0;
unsigned long engineTimer = 0;
unsigned long dashboardTimer = 0;
unsigned long wireTransferTimer = 0;
long dashboardPeriod = 10000;
long wireTransferPeriod = 500;
#pragma endregion

#pragma region Diagnostic vars
//--KWP
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);
//-----

//---Diagnostics---//
#define MAX_CONNECT_RETRIES 5
#define NENGINEGROUPS 4
#define NDASHBOARDGROUPS 1
#define NMODULES 2
int engineGroups[NENGINEGROUPS] = {1, 2, 4, 16};
int dashboardGroups[NDASHBOARDGROUPS] = {3};

KWP_MODULE engine = {"ECU", ADR_Engine, 10400, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"DASHBOARD", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
KWP_MODULE *modules[NMODULES] = {&engine, &dashboard};

KWP_MODULE *currentModule = modules[0];
int currentGroup = 0;
int currentSensor = 0;
int nSensors = 0;
int maxSensors = 4;
int connRetries = 0;
int count = 0;
//
#pragma endregion

#pragma region WireTransfer vars
//---Communication between Arduino and Seeeduino---//
//Library: EasyTransfer (https://github.com/madsci1016/Arduino-EasyTransfer)//
EasyTransferI2C ET;

//define slave i2c address
#define I2C_SLAVE_ADDRESS 9

struct SEND_DATA_STRUCTURE
{
  bool connectionStatus = false;
  String oilTemp = "";
  String coolantTemp = "";
  String MAF = "";
  String misfireCounter = "";
  String batteryVoltage = "";
};

//give a name to the group of data
SEND_DATA_STRUCTURE wireData;
#pragma endregion

void setup()
{
  setupWireTransfer();
  setupDiagnostic();
}

void loop()
{
  Serial.print("Loop elapsed: ");
  Serial.println(millis() - loopTimer);
  loopTimer = millis();

  manageWireTransfer();
  manageDiagnostic();

  //showWireDataToSerial();
}

#pragma region Diagnostic Handlers
//Diagnostic handlers
// 0 - Nothing
// 1 - Up
// 2 - Down
void refreshParams(int type)
{
  if (type == 1)
  {
    if (currentSensor < nSensors - 1)
      currentSensor++;
    else
    {
      currentSensor = 0;
      if (currentGroup < (currentModule->ngroups) - 1)
        currentGroup++;
      else
      {
        if (currentModule->addr == ADR_Dashboard)
          currentModule = modules[1];
        else
          currentModule = modules[0];
        currentGroup = 0;
        kwp.disconnect();
      }
    }
  }
  else if (type == 2)
  {
    if (currentSensor > 0)
      currentSensor--;
    else
    {
      currentSensor = nSensors - 1;
      if (currentGroup > 0)
        currentGroup--;
      else
      {
        if (currentModule->addr == ADR_Dashboard)
          currentModule = modules[1];
        else
          currentModule = modules[0];
        currentGroup = currentModule->ngroups - 1;
        kwp.disconnect();
      }
    }
  }
}

void setupDiagnostic()
{
  bool result = kwp.connect(currentModule->addr, currentModule->baudrate);
}

void manageDiagnostic()
{
  if (millis() - dashboardTimer > dashboardPeriod)
  {
    currentModule = modules[1];
    kwp.disconnect();
  }
  if (kwp.isConnected())
  {
    if (currentModule->name == "ECU")
    {
      SENSOR resultBlock[maxSensors];
      kwp.readBlock(currentModule->addr, currentModule->groups[1], maxSensors, resultBlock);
      wireData.coolantTemp = resultBlock[1].value;
      kwp.readBlock(currentModule->addr, currentModule->groups[2], maxSensors, resultBlock);
      wireData.MAF = resultBlock[3].value;
      kwp.readBlock(currentModule->addr, currentModule->groups[16], maxSensors, resultBlock);
      wireData.misfireCounter = resultBlock[0].value;
      kwp.readBlock(currentModule->addr, currentModule->groups[51], maxSensors, resultBlock);
      wireData.batteryVoltage = resultBlock[3].value;
    }
    else if (currentModule->name == "DASHBOARD")
    {
      SENSOR resultBlock[maxSensors];
      kwp.readBlock(currentModule->addr, currentModule->groups[3], maxSensors, resultBlock);
      wireData.oilTemp = resultBlock[2].value;
      dashboardTimer = millis();
    }
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Starting " + currentModule->name);
    if (kwp.connect(currentModule->addr, currentModule->baudrate))
    {
      Serial.println("Con. OK! Reading...");
      connRetries = 0;
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    { // Antiblocking
      Serial.println("KWP not connected");
      digitalWrite(LED_BUILTIN, LOW);
      if (connRetries > MAX_CONNECT_RETRIES)
      {
        Serial.println("Connection retry " + String(connRetries) + "... Module addr " + String(currentModule->addr));
        if (currentModule->addr == ADR_Dashboard)
        {
          dashboardTimer = millis();
          currentModule = modules[0];
        }
        else
          currentModule = modules[1];
        currentGroup = 0;
        currentSensor = 0;
        nSensors = 0;
        connRetries = 0;
      }
      else
        connRetries++;
    }
  }
}
//
#pragma endregion

#pragma region WireTransfer Handlers
//Arduino Transfer digital wire
void setupWireTransfer()
{
  Wire.begin();
  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc.
  ET.begin(details(wireData), &Wire);

  pinMode(13, OUTPUT);

  randomSeed(analogRead(0));
}

void manageWireTransfer()
{
  wireTransferTimer = millis();
  while (millis() < wireTransferTimer + wireTransferPeriod)
  {
    //send the data
    ET.sendData(I2C_SLAVE_ADDRESS);
    Serial.println("Wire Transfer in progress");
  }
}
//
#pragma endregion

void showWireDataToSerial()
{
  Serial.print("Oil temp: ");
  Serial.print(wireData.oilTemp);
  Serial.println();
  Serial.print("Coolant temp: ");
  Serial.print(wireData.coolantTemp);
  Serial.println();
  Serial.print("MAF: ");
  Serial.print(wireData.MAF);
  Serial.println();
  Serial.print("Misfires: ");
  Serial.print(wireData.misfireCounter);
  Serial.println();
  Serial.print("Batt voltage: ");
  Serial.print(wireData.batteryVoltage);
  Serial.println();
}