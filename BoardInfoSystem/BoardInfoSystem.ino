#include <EasyTransferI2C.h>
#include <Wire.h>

#pragma region Diagnostic vars
EasyTransferI2C ET; 

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int16_t blinks;
  int16_t pause;
};

//give a name to the group of data
SEND_DATA_STRUCTURE mydata;

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
  ET.begin(details(mydata), &Wire);
  Wire.onReceive(receive);
  pinMode(13, OUTPUT);
}

void manageWireTransfer()
{
  if(ET.receiveData()){
    //this is how you access the variables. [name of the group].[variable name]
    //since we have data, we will blink it out. 
    for(int i = mydata.blinks; i>0; i--){
      digitalWrite(13, HIGH);
      delay(mydata.pause * 100);
      digitalWrite(13, LOW);
      delay(mydata.pause * 100);
    }
  }
}

void receive(int numBytes){
}
//
#pragma endregion