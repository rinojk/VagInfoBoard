#include <Wire.h>

struct DIAG_DATA_STRUCTURE{
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
struct DIAG_DATA_STRUCTURE mydata;

void setup()
{
  Serial.begin(115200);
  Wire.begin(9);                // join i2c bus with address #2
  Wire.onRequest(requestEvent); // register event
}

void loop()
{
  delay(10);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Serial.println("Request came; DataStruct size:");
  Serial.println(sizeof(&mydata));
  Wire.write(17); // respond with message of 6 bytes
                       // as expected by master
}