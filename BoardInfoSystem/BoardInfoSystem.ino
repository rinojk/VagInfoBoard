#include <EasyTransferI2C.h>
#include <Wire.h>

struct SEND_DATA_STRUCTURE
{
  bool connectionStatus = false;
  String oilTemp = "";
  String coolantTemp = "";
  String MAF = "";
  String misfireCounter = "";
  String batteryVoltage = "";
};

#pragma region WireTransfer vars
EasyTransferI2C ET; 

//give a name to the group of data
SEND_DATA_STRUCTURE wireData;

//define slave i2c address
#define I2C_SLAVE_ADDRESS 9
#pragma endregion

void setup() {
  setupWireTransfer();
}

void loop() {
  manageWireTransfer();
}

#pragma region WireTransfer Handlers
//Arduino Transfer digital wire
void setupWireTransfer()
{
  Wire.begin(I2C_SLAVE_ADDRESS);
  ET.begin(details(wireData), &Wire);
  Wire.onReceive(receive);
  pinMode(13, OUTPUT);
}

void manageWireTransfer()
{
  if(ET.receiveData()){
    
  }
}

void receive(int numBytes){
}
//
#pragma endregion