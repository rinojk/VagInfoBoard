#include <Wire.h>
#include "i2cSimpleTransfer.h"

/*
    https://github.com/getsurreal/i2cSimpleTransfer

    This example simulates a Slave Arduino sending data to the Master upon request from the Master.

    In addition, the Slave receives configuration changes sent from the Master.

    We start with a simulated sensor value.  Everytime the Master requests the value we increment the value.

    Initially the value is incrementing by one.
    When the Master recieves the 100th request, it sends a configuration change to the Slave to start incrementing by 100.

*/

#define i2c_slave_address 17

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
    int16_t sensor = 0; // use specific declarations to ensure communication between 16bit and 32bit controllers
    int16_t connectionStatus = 1;
    int16_t oilTemp = 89;
    int16_t coolantTemp = 90;
    int16_t MAF = 91;
    int16_t misfireCounter = 92;
    int16_t batteryVoltage = 93;
};

struct SLAVE_CONFIG
{
    uint8_t val; // use specific declarations to ensure communication between 16bit and 32bit controllers
};

SLAVE_DATA slave_data;
SLAVE_CONFIG slave_config;

void setup()
{
    Wire.begin(i2c_slave_address); // i2c Slave address
    Wire.onRequest(requestEvent);  // when the Master makes a request, run this function
    Wire.onReceive(receiveEvent);  // when the Master sends us data, run this function
    slave_config.val = 1;          // This is how much we increment after each request
}

void loop()
{
}

void requestEvent()
{
    i2cSimpleWrite(slave_data);            // Send the Master the sensor data
    //slave_data.sensor += slave_config.val; // Simulate updated sensor data
}

void receiveEvent(int payload)
{
    if (payload == sizeof(slave_config))
    {
        i2cSimpleRead(slave_config); // Receive new data from the Master
    }
}