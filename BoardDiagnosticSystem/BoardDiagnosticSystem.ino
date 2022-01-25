#include <EasyTransferI2C.h>
#include <Wire.h>
#include "KWP.h"

//--KWP
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);
//-----

//---Communication between Arduino and Seeeduino---//
//Library: EasyTransfer (https://github.com/madsci1016/Arduino-EasyTransfer)//
EasyTransferI2C ET; 

//define slave i2c address
#define I2C_SLAVE_ADDRESS 9

//sample data structure. To be changed
struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int16_t blinks;
  int16_t pause;
};

//give a name to the group of data
SEND_DATA_STRUCTURE mydata;

void setup(){
  
}

void loop(){

}