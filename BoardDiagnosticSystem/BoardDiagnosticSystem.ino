#include <EasyTransferI2C.h>
#include <Wire.h>
#include "KWP.h"

#pragma region Timers
long timer1 = 0;
long timer2 = 0;
long timer3 = 0;
#pragma endregion

#pragma region Diagnostic vars
//--KWP
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);
//-----

//---Diagnostics---//
#define MAX_CONNECT_RETRIES 5
#define NENGINEGROUPS 3
#define NDASHBOARDGROUPS 2
#define NMODULES 2
int engineGroups[NENGINEGROUPS] = {2, 3, 20};
int dashboardGroups[NDASHBOARDGROUPS] = {1, 3};

KWP_MODULE engine = {"ECU", ADR_Engine, 10400, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"DASHBOARD", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
KWP_MODULE *modules[NMODULES] = {&dashboard, &engine};

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

//sample data structure. To be changed
struct SEND_DATA_STRUCTURE
{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int16_t blinks;
  int16_t pause;
};

//give a name to the group of data
SEND_DATA_STRUCTURE mydata;
#pragma endregion

void setup()
{
  setupWireTransfer();
  setupDiagnostic();
}

void loop()
{
  manageWireTransfer();
  manageDiagnostic();
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
  if (!kwp.isConnected())
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
          currentModule = modules[1];
        else
          currentModule = modules[0];
        currentGroup = 0;
        currentSensor = 0;
        nSensors = 0;
        connRetries = 0;
      }
      else
        connRetries++;
    }
  }
  else
  {
    digitalWrite(LED_BUILTIN, HIGH);
    SENSOR resultBlock[maxSensors];
    nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);
    if (resultBlock[currentSensor].value != "")
    {
      //LCD.showText(resultBlock[currentSensor].desc, resultBlock[currentSensor].value+" "+resultBlock[currentSensor].units);
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println(resultBlock[currentSensor].desc);
      Serial.println(resultBlock[currentSensor].value + " " + resultBlock[currentSensor].units);
      if (count > 8)
      {
        refreshParams(1);
        count = 0;
      }
      else
        count++;
    }
    else
    {
      refreshParams(1);
      count = 0;
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
  ET.begin(details(mydata), &Wire);

  pinMode(13, OUTPUT);

  randomSeed(analogRead(0));
}

void manageWireTransfer()
{
  //this is how you access the variables. [name of the group].[variable name]
  mydata.blinks = random(5);
  mydata.pause = random(5);
  //send the data
  ET.sendData(I2C_SLAVE_ADDRESS);

  //Just for fun, we will blink it out too
  for (int i = mydata.blinks; i > 0; i--)
  {
    digitalWrite(13, HIGH);
    delay(mydata.pause * 100);
    digitalWrite(13, LOW);
    delay(mydata.pause * 100);
  }
  delay(5000);
}
//
#pragma endregion