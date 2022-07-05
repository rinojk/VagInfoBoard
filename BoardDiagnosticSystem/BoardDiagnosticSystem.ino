#include "KWP.h"

#define MAX_CONNECT_RETRIES 5
#define NENGINEGROUPS 5
//#define NDASHBOARDGROUPS 1
#define NMODULES 1

// KWP
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);

int engineGroups[NENGINEGROUPS] = { 2, 3, 4, 5, 134};
//int dashboardGroups[NDASHBOARDGROUPS] = { 2 };

// Note: Each unit can have his own baudrate.
// If 10400 not works whit your unit try whit other, posible values are:
// 115200, 57600, 38400, 31250, 28800, 19200,  14400, 10400, 9600, 4800, 2400, 1200, 600, 300

KWP_MODULE engine    = { "ECU",     ADR_Engine,    10400, engineGroups,    NENGINEGROUPS};
//KWP_MODULE dashboard = { "CLUSTER", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
KWP_MODULE *modules[NMODULES]={ &engine };

KWP_MODULE *currentModule=modules[0];
int currentGroup=0;
int currentSensor=0;
int nSensors=0;
int maxSensors=4;
int connRetries=0;

int count=0;

// Will return:
// 0 - Nothing
// 1 - Key Up pressed
// 2 - Key Down pressed
int getKeyStatusRandom(){
  int key1=random(0,2);
  int key2=random(0,2);
  if(key1 == 1 && key2 == 0) return 1;
  if(key1 == 0 && key2 == 1) return 2;
  if(key1 == 0 && key2 == 0) return 0;
  if(key1 == 1 && key2 == 1){
    if(random(0,2)==1) return 1;
    else return 2;
  }
}

// 0 - Nothing
// 1 - Up
// 2 - Down
void refreshParams(int type){
  if(type==1){
    if(currentSensor < nSensors -1) currentSensor++;
    else{
      currentSensor=0;
      if(currentGroup < (currentModule->ngroups) - 1) currentGroup++;
      else{
        //if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        //else currentModule=modules[0];
        currentGroup=0;
        kwp.disconnect();
      }
    }
  }
  else if(type==2){
    if(currentSensor > 0) currentSensor--;
    else{
      currentSensor=nSensors-1;
      if(currentGroup > 0) currentGroup--;
      else{
        //if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        //else currentModule=modules[0];
        currentGroup=currentModule->ngroups-1;
        kwp.disconnect();
      }
    }
  }
}

void setup(){
  Serial.begin(9600);
  for(int i=0; i<8; i++){
    delay(500);
  }
  for(int i=0; i<8; i++){
    delay(500);
  }
}

void loop(){
  if(!kwp.isConnected()){
    if(kwp.connect(currentModule->addr, currentModule->baudrate)){
      connRetries=0;
    }
    else{ // Antiblocking
      if(connRetries > MAX_CONNECT_RETRIES){
        //if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        //else currentModule=modules[0];
        currentGroup=0;
        currentSensor=0;
        nSensors=0;
        connRetries=0;
      }
      else connRetries++;
    }

  }
  else{
    SENSOR resultBlock[maxSensors];
    nSensors=kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);
    if(resultBlock[currentSensor].value != ""){
      Serial.println("---BLOCK SHOW INFO---");
      Serial.println("CurrGroup: "+String(engineGroups[currentGroup])+"; CurrSensor: "+String(currentSensor));
      Serial.println("type: "+String(resultBlock[currentSensor].type)+"; a: "+String(resultBlock[currentSensor].type)+"; b: "+String(resultBlock[currentSensor].b));
      Serial.println(resultBlock[currentSensor].desc+" "+resultBlock[currentSensor].value+" "+resultBlock[currentSensor].units);
      Serial.println("---------------------");
      if(count>8){
        refreshParams(1);
        count=0;
      }
      else count++;
    }
    else{
      refreshParams(1);
      count=0;
    }
  }
}