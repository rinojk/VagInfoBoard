#include "KWP.h"

#include <Wire.h>
#include "i2cSimpleTransfer.h"

#pragma region I2Cmanaging
#define i2c_slave_address 17

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
    int16_t sensor = 0; // use specific declarations to ensure communication between 16bit and 32bit controllers
    int16_t connectionStatus = 0;
    int16_t oilTemp = -99;
    int16_t coolantTemp = -99;
    int16_t MAF = -99;
    int16_t misfireCounter = -99;
    int16_t batteryVoltage = -99;
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
    //slave_data.oilTemp += 1;
    //slave_data.batteryVoltage += 2;
    i2cSimpleWrite(slave_data); // Send the Master the sensor data
    //showWireDataToSerial();
    //slave_data.sensor += slave_config.val; // Simulate updated sensor data
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
    
    if (kwp.isConnected())
    {
        if (currentModule->name == "ECU")
        {
            SENSOR resultBlock[maxSensors];
            kwp.readBlock(currentModule->addr, currentModule->groups[1], maxSensors, resultBlock);
            //wireData.coolantTemp = resultBlock[1].value;
            Serial.println(resultBlock[1].value+" "+resultBlock[1].units+" "+resultBlock[1].desc);
            if(resultBlock[1].value!=""){
                slave_data.coolantTemp = (int)resultBlock[1].value.toFloat();
            }
            kwp.readBlock(currentModule->addr, currentModule->groups[2], maxSensors, resultBlock);
            //wireData.MAF = resultBlock[3].value;
            Serial.println(resultBlock[3].value+" "+resultBlock[3].units+" "+resultBlock[3].desc);
            if(resultBlock[3].value!=""){
                slave_data.MAF = (int)(resultBlock[3].value.toFloat()*100);
            }
            kwp.readBlock(currentModule->addr, currentModule->groups[16], maxSensors, resultBlock);
            //wireData.misfireCounter = resultBlock[0].value;
            Serial.println(resultBlock[0].value+" "+resultBlock[0].units+" "+resultBlock[0].desc);
            if(resultBlock[0].value!=""){
                slave_data.misfireCounter = (int)(resultBlock[0].value.toFloat());
            }
            kwp.readBlock(currentModule->addr, currentModule->groups[51], maxSensors, resultBlock);
            //wireData.batteryVoltage = resultBlock[3].value;
            Serial.println(resultBlock[3].value+" "+resultBlock[3].units+" "+resultBlock[3].desc);
            if(resultBlock[3].value!=""){
                slave_data.batteryVoltage = (int)(resultBlock[3].value.toFloat()*100);
            }
        }
        else if (currentModule->name == "DASHBOARD")
        {
            SENSOR resultBlock[maxSensors];
            kwp.readBlock(currentModule->addr, currentModule->groups[1], maxSensors, resultBlock);
            //wireData.oilTemp = resultBlock[2].value;
            Serial.println(resultBlock[2].value+" "+resultBlock[2].units+" "+resultBlock[2].desc);
            if(resultBlock[2].value!=""){
                slave_data.oilTemp = (int)(resultBlock[2].value.toFloat());
            }
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
    Serial.print("Batt voltage: ");
    Serial.print(slave_data.batteryVoltage);
    Serial.println();
}