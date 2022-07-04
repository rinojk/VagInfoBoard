#include "KWP.h"

#include <Wire.h>
#include "i2cSimpleTransfer.h"

#pragma region I2Cmanaging
#define i2c_slave_address 17

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
  // TO BE RENAMED TO diagConnectionStatus
  //  0: not connected
  //  1: connected to ECU
  //  2: connected to DASH
  int16_t sensor = 0; // use specific declarations to ensure communication between 16bit and 32bit controllers
  // TO BE RENAMED TO bridgeConnectionStatus
  //  0: not connected
  //  1: connected
  int16_t connectionStatus = 1;
  int16_t oilTemp = 91;
  int16_t coolantTemp = 92;
  int16_t MAF = 0;
  int16_t misfireCounter = 0;
  int16_t intakeAirTemp = 93;
};

struct SLAVE_CONFIG
{
    uint8_t val; // use specific declarations to ensure communication between 16bit and 32bit controllers
};

SLAVE_DATA slave_data;
SLAVE_CONFIG slave_config;
void setupI2C()
{
    Wire.begin(i2c_slave_address); // i2c Slave address
    Wire.onRequest(requestEvent);  // when the Master makes a request, run this function
    Wire.onReceive(receiveEvent);  // when the Master sends us data, run this function
    slave_config.val = 1;          // This is how much we increment after each request
}
void requestEvent()
{
    // slave_data.oilTemp += 1;
    // slave_data.batteryVoltage += 2;
    i2cSimpleWrite(slave_data); // Send the Master the sensor data
    // showWireDataToSerial();
    // slave_data.sensor += slave_config.val; // Simulate updated sensor data
}

void receiveEvent(int payload)
{
    if (payload == sizeof(slave_config))
    {
        i2cSimpleRead(slave_config); // Receive new data from the Master
    }
}
#pragma endregion

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
int engineGroups[NENGINEGROUPS] = {2, 3, 20, 31};
int dashboardGroups[NDASHBOARDGROUPS] = {2};

KWP_MODULE engine = {"ECU", ADR_Engine, 10400, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"CLUSTER", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
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

void setup()
{
    Serial.begin(9600);
    setupI2C();
    setupDiagnostic();
}

void loop()
{
    Serial.print("Loop elapsed: ");
    Serial.println(millis() - loopTimer);
    loopTimer = millis();

    manageDiagnostic();

    // showWireDataToSerial();
}

#pragma region Diagnostic Handlers
// Diagnostic handlers
//  0 - Nothing
//  1 - Up
//  2 - Down
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
    //bool result = kwp.connect(currentModule->addr, currentModule->baudrate);
}

void manageDiagnostic()
{
    
    if (!kwp.isConnected())
    {
        slave_data.connectionStatus=0;
        Serial.println("Starting " + currentModule->name);
        if (kwp.connect(currentModule->addr, currentModule->baudrate))
        {
            Serial.println("Con. OK! Reading...");
            connRetries = 0;
        }
        else
        { // Antiblocking
            Serial.println("KWP not connected");
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
        slave_data.connectionStatus=1;
        SENSOR resultBlock[maxSensors];
        nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);
        if (resultBlock[currentSensor].value != "")
        {
            // LCD.showText(resultBlock[currentSensor].desc, resultBlock[currentSensor].value+" "+resultBlock[currentSensor].units);
            Serial.println("CurrentGroup: "+String(currentGroup)+"; CurrentSensor: "+String(currentSensor));
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

void showWireDataToSerial()
{
    Serial.print("Oil temp: ");
    Serial.print(slave_data.oilTemp);
    Serial.println();
    Serial.print("Coolant temp: ");
    Serial.print(slave_data.coolantTemp);
    Serial.println();
    Serial.print("MAF: ");
    Serial.print(slave_data.MAF);
    Serial.println();
    Serial.print("Misfires: ");
    Serial.print(slave_data.misfireCounter);
    Serial.println();
    Serial.print("Intake Air Temp: ");
    Serial.print(slave_data.intakeAirTemp);
    Serial.println();
}