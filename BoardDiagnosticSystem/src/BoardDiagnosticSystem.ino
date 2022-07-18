/*
Arduino Nano OBD reader (OBD protocol KW1281,  Audi A4 B5 etc.)

wiring:
D2 --- OBD level shifter input (RX) (e.g. LM339)
D3 --- OBD level shifter output (TX) (e.g. LM339)
HD44780 default wiring

NOTE: For the level shifting, I used a 'AutoDia K409 Profi USB adapter', disassembled it,
      and connected the Arduino to the level shifter chip (LM339) - the original FTDI chip TX line
      was removed (so it does not influence the communication)

NOTE2: all serial debug communication commented out for live version, uncomment if you want it

*/

#include <EEPROM.h>
#include "NewSoftwareSerial.h"

#include <Wire.h>
#include "i2cSimpleTransfer.h"

#define pinKLineRX 2 // white
#define pinKLineTX 3 // orange
//#define pinLED 9
//#define pinBuzzer 5
//#define pinButton 8

// https://www.blafusel.de/obd/obd2_kw1281.html

#define ADR_Engine 0x01
//#define ADR_Gears  0x02
#define ADR_ABS_Brakes 0x03
#define ADR_Airbag 0x15
#define ADR_Dashboard 0x17
#define ADR_Immobilizer 0x25
#define ADR_Central_locking 0x35

//#define DEBUG 1

NewSoftwareSerial obd(pinKLineRX, pinKLineTX, false); // RX, TX, inverse logic

uint8_t currAddr = 0;
uint8_t blockCounter = 0;
unsigned long errorTimeout = 0;
unsigned long errorData = 0;
bool connected = false;
int sensorCounter = 0;
int pageUpdateCounter = 0;
int alarmCounter = 0;

uint8_t currPage = 1;

int8_t coolantTemp = 0;
int8_t oilTemp = 0;
int8_t intakeAirTemp = 0;
int8_t oilPressure = 0;
uint8_t engineLoad = 0;
uint8_t engineSpeed = 0;
float throttleValve = 0;
float supplyVoltage = 0;
uint8_t vehicleSpeed = 0;
float fuelConsumption = 0;
uint8_t fuelLevel = 0;
unsigned long odometer = 0;

float actTrip = 0.0;
float actFuel = 0.0;
unsigned long tripTime = 0UL;
unsigned long fuelTime = 0UL;
float tripDist = 0;   //[m]
uint8_t tripVmax = 0; //[Km/h]
uint8_t vMax = 0;
uint8_t vMaxAddr = 0;
long lastMillis = 0;
uint8_t lastVehicleSpeed = 0;
uint8_t deltaV = 0;
long deltaT = 0;
const float distCorrVal = 1.03643724696; // correction value for distance counter: = realDistance/measuredDistance

#pragma region I2Cmanaging
#define i2c_slave_address 17

// You can add more variables into the struct, but the default limit for transfer size in the Wire library is 32 bytes
struct SLAVE_DATA
{
  // TO BE RENAMED TO diagConnectionStatus
  //  0: not connected
  //  1: connected to ECU
  //  2: connected to DASH
  int16_t diagConnectionStatus = 0; // use specific declarations to ensure communication between 16bit and 32bit controllers
  // TO BE RENAMED TO bridgeConnectionStatus
  //  0: not connected
  //  1: connected
  int16_t connectionStatus = 0;
  int16_t rpm = 0;
  int16_t vehicleSpeed = 0;
  int16_t oilTemp = 0;
  int16_t coolantTemp = 0;
  int16_t MAF = 0;
  int16_t misfireCounter = 0;
  int16_t intakeAirTemp = 0;
  int16_t batteryVoltage = 0;
  int16_t isEngineWorking = 0;
  int16_t fuelLevel = 0;
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

#pragma region KWP METHODS
String floatToString(float v)
{
  String res;
  char buf[16];
  dtostrf(v, 4, 2, buf);
  res = String(buf);
  return res;
}

void disconnect()
{
  connected = false;
  currAddr = 0;
}

void obdWrite(uint8_t data)
{
#ifdef DEBUG
/*  Serial.print("uC:");
  Serial.println(data, HEX);*/
#endif

  //*********************  ADDED 5ms delay ****************************************************************
  // delay(5);
  delay(4); // pimp
  obd.write(data);
}

uint8_t obdRead()
{
  unsigned long timeout = millis() + 1000;
  while (!obd.available())
  {
    if (millis() >= timeout)
    {
      //      Serial.println(F("ERROR: obdRead timeout"));
      disconnect();
      errorTimeout++;
      return 0;
    }
  }
  uint8_t data = obd.read();
#ifdef DEBUG
//  Serial.print("ECU:");
//  Serial.println(data, HEX);
#endif
  return data;
}

// 5Bd, 7O1
void send5baud(uint8_t data)
{
  // // 1 start bit, 7 data bits, 1 parity, 1 stop bit
#define bitcount 10
  byte bits[bitcount];
  byte even = 1;
  byte bit;
  for (int i = 0; i < bitcount; i++)
  {
    bit = 0;
    if (i == 0)
      bit = 0;
    else if (i == 8)
      bit = even; // computes parity bit
    else if (i == 9)
      bit = 1;
    else
    {
      bit = (byte)((data & (1 << (i - 1))) != 0);
      even = even ^ bit;
    }
    /*    Serial.print(F("bit"));
        Serial.print(i);
        Serial.print(F("="));
        Serial.print(bit);
        if (i == 0) Serial.print(F(" startbit"));
        else if (i == 8) Serial.print(F(" parity"));
        else if (i == 9) Serial.print(F(" stopbit"));
        Serial.println();*/
    bits[i] = bit;
  }
  // now send bit stream
  for (int i = 0; i < bitcount + 1; i++)
  {
    if (i != 0)
    {
      // wait 200 ms (=5 baud), adjusted by latency correction
      delay(200);
      if (i == bitcount)
        break;
    }
    if (bits[i] == 1)
    {
      // high
      digitalWrite(pinKLineTX, HIGH);
    }
    else
    {
      // low
      digitalWrite(pinKLineTX, LOW);
    }
  }
  obd.flush();
}

bool KWP5BaudInit(uint8_t addr)
{
  //  Serial.println(F("---KWP 5 baud init"));
  // delay(3000);
  send5baud(addr);
  return true;
}

bool KWPSendBlock(char *s, int size)
{
  /*  Serial.print(F("---KWPSend sz="));
    Serial.print(size);
    Serial.print(F(" blockCounter="));
    Serial.println(blockCounter);
    // show data
    Serial.print(F("OUT:"));*/
  for (int i = 0; i < size; i++)
  {
    uint8_t data = s[i];
    //    Serial.print(data, HEX);
    //    Serial.print(" ");
  }
  //  Serial.println();
  for (int i = 0; i < size; i++)
  {
    uint8_t data = s[i];
    obdWrite(data);
    /*uint8_t echo = obdRead();
    if (data != echo){
      Serial.println(F("ERROR: invalid echo"));
      disconnect();
      errorData++;
      return false;
    }*/
    if (i < size - 1)
    {
      uint8_t complement = obdRead();
      if (complement != (data ^ 0xFF))
      {
        //        Serial.println(F("ERROR: invalid complement"));
        disconnect();
        errorData++;
        return false;
      }
    }
  }
  blockCounter++;
  return true;
}

// count: if zero given, first received byte contains block length
// 4800, 9600 oder 10400 Baud, 8N1
bool KWPReceiveBlock(char s[], int maxsize, int &size)
{
  bool ackeachbyte = false;
  uint8_t data = 0;
  int recvcount = 0;
  if (size == 0)
    ackeachbyte = true;
  /*  Serial.print(F("---KWPReceive sz="));
    Serial.print(size);
    Serial.print(F(" blockCounter="));
    Serial.println(blockCounter); */
  if (size > maxsize)
  {
    //    Serial.println("ERROR: invalid maxsize");
    return false;
  }
  unsigned long timeout = millis() + 1000;
  while ((recvcount == 0) || (recvcount != size))
  {
    while (obd.available())
    {
      data = obdRead();
      s[recvcount] = data;
      recvcount++;
      if ((size == 0) && (recvcount == 1))
      {
        size = data + 1;
        if (size > maxsize)
        {
          //          Serial.println("ERROR: invalid maxsize");
          return false;
        }
      }
      if ((ackeachbyte) && (recvcount == 2))
      {
        if (data != blockCounter)
        {
          //          Serial.println(F("ERROR: invalid blockCounter"));
          disconnect();
          errorData++;
          return false;
        }
      }
      if (((!ackeachbyte) && (recvcount == size)) || ((ackeachbyte) && (recvcount < size)))
      {
        obdWrite(data ^ 0xFF); // send complement ack
        /*uint8_t echo = obdRead();
        if (echo != (data ^ 0xFF)){
          Serial.print(F("ERROR: invalid echo "));
          Serial.println(echo, HEX);
          disconnect();
          errorData++;
          return false;
        }*/
      }
      timeout = millis() + 1000;
    }
    if (millis() >= timeout)
    {
      //      Serial.println(F("ERROR: timeout"));
      disconnect();
      errorTimeout++;
      return false;
    }
  }
  // show data
  /*  Serial.print(F("IN: sz="));
    Serial.print(size);
    Serial.print(F(" data=")); */
  for (int i = 0; i < size; i++)
  {
    uint8_t data = s[i];
    /*    Serial.print(data, HEX);
        Serial.print(F(" "));*/
  }
  //  Serial.println();
  blockCounter++;
  return true;
}

bool KWPSendAckBlock()
{
  /*  Serial.print(F("---KWPSendAckBlock blockCounter="));
    Serial.println(blockCounter);*/
  char buf[32];
  sprintf(buf, "\x03%c\x09\x03", blockCounter);
  return (KWPSendBlock(buf, 4));
}

bool connect(uint8_t addr, int baudrate)
{
  Serial.print(F("------connect addr="));
  Serial.print(addr);
  Serial.print(F(" baud="));
  Serial.println(baudrate);
  // tone(pinBuzzer, 1200);
  delay(100);
  // noTone(pinBuzzer);
  /*  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("KW1281 wakeup");
    //  lcdPrint(0, 1, "", 20);
    //  lcdPrint(0, 2, "", 20);*/
  blockCounter = 0;
  currAddr = 0;
  obd.begin(baudrate);
  KWP5BaudInit(addr);
  // answer: 0x55, 0x01, 0x8A
  char s[3];
  /*  lcd.setCursor(0, 0);
    lcd.print("KW1281 recv"); */
  int size = 3;
  if (!KWPReceiveBlock(s, 3, size))
    return false;
  if ((((uint8_t)s[0]) != 0x55) || (((uint8_t)s[1]) != 0x01) || (((uint8_t)s[2]) != 0x8A))
  {
    //    Serial.println(F("ERROR: invalid magic"));
    disconnect();
    errorData++;
    return false;
  }
  currAddr = addr;
  connected = true;
  if (!readConnectBlocks())
    return false;
  return true;
}

bool readConnectBlocks()
{
  // read connect blocks
  //  Serial.println(F("------readconnectblocks"));
  /*  lcd.setCursor(0, 0);
    lcd.print("KW1281 label");*/
  String info;
  while (true)
  {
    int size = 0;
    char s[64];
    if (!(KWPReceiveBlock(s, 64, size)))
      return false;
    if (size == 0)
      return false;
    if (s[2] == '\x09')
      break;
    if (s[2] != '\xF6')
    {
      //      Serial.println(F("ERROR: unexpected answer"));
      disconnect();
      errorData++;
      return false;
    }
    String text = String(s);
    info += text.substring(3, size - 2);
    if (!KWPSendAckBlock())
      return false;
  }
  //  Serial.print("label=");
  //  Serial.println(info);
  // lcd.setCursor(0, 1);
  // lcd.print(info);
  return true;
}

bool readSensors(int group)
{
  //  Serial.print(F("------readSensors "));
  //  Serial.println(group);
  // lcd.setCursor(0, 0);
  // lcd.print("KW1281 sensor");
  char s[64];
  sprintf(s, "\x04%c\x29%c\x03", blockCounter, group);
  if (!KWPSendBlock(s, 5))
    return false;
  int size = 0;
  KWPReceiveBlock(s, 64, size);
  if (s[2] != '\xe7')
  {
    //    Serial.println(F("ERROR: invalid answer"));
    disconnect();
    errorData++;
    return false;
  }
  int count = (size - 4) / 3;
  //  Serial.print(F("count="));
  //  Serial.println(count);
  for (int idx = 0; idx < count; idx++)
  {
    byte k = s[3 + idx * 3];
    byte a = s[3 + idx * 3 + 1];
    byte b = s[3 + idx * 3 + 2];
    String n;
    float v = 0;
    /*    Serial.print(F("type="));
        Serial.print(k);
        Serial.print(F("  a="));
        Serial.print(a);
        Serial.print(F("  b="));
        Serial.print(b);
        Serial.print(F("  text="));*/
    String t = "";
    String units = "";
    char buf[32];
    switch (k)
    {
    case 1:
      v = 0.2 * a * b;
      units = F("rpm");
      break;
    case 2:
      v = a * 0.002 * b;
      units = F("%%");
      break;
    case 3:
      v = 0.002 * a * b;
      units = F("Deg");
      break;
    case 4:
      v = abs(b - 127) * 0.01 * a;
      units = F("ATDC");
      break;
    case 5:
      v = a * (b - 100) * 0.1;
      units = F("°C");
      break;
    case 6:
      v = 0.001 * a * b;
      units = F("V");
      break;
    case 7:
      v = 0.01 * a * b;
      units = F("km/h");
      break;
    case 8:
      v = 0.1 * a * b;
      units = F(" ");
      break;
    case 9:
      v = (b - 127) * 0.02 * a;
      units = F("Deg");
      break;
    case 10:
      if (b == 0)
        t = F("COLD");
      else
        t = F("WARM");
      break;
    case 11:
      v = 0.0001 * a * (b - 128) + 1;
      units = F(" ");
      break;
    case 12:
      v = 0.001 * a * b;
      units = F("Ohm");
      break;
    case 13:
      v = (b - 127) * 0.001 * a;
      units = F("mm");
      break;
    case 14:
      v = 0.005 * a * b;
      units = F("bar");
      break;
    case 15:
      v = 0.01 * a * b;
      units = F("ms");
      break;
    case 18:
      v = 0.04 * a * b;
      units = F("mbar");
      break;
    case 19:
      v = a * b * 0.01;
      units = F("l");
      break;
    case 20:
      v = a * (b - 128) / 128;
      units = F("%%");
      break;
    case 21:
      v = 0.001 * a * b;
      units = F("V");
      break;
    case 22:
      v = 0.001 * a * b;
      units = F("ms");
      break;
    case 23:
      v = b / 256 * a;
      units = F("%%");
      break;
    case 24:
      v = 0.001 * a * b;
      units = F("A");
      break;
    case 25:
      v = (b * 1.421) + (a / 182);
      units = F("g/s");
      break;
    case 26:
      v = float(b - a);
      units = F("C");
      break;
    case 27:
      v = abs(b - 128) * 0.01 * a;
      units = F("°");
      break;
    case 28:
      v = float(b - a);
      units = F(" ");
      break;
    case 30:
      v = b / 12 * a;
      units = F("Deg k/w");
      break;
    case 31:
      v = b / 2560 * a;
      units = F("°C");
      break;
    case 33:
      v = 100 * b / a;
      units = F("%%");
      break;
    case 34:
      v = (b - 128) * 0.01 * a;
      units = F("kW");
      break;
    case 35:
      v = 0.01 * a * b;
      units = F("l/h");
      break;
    case 36:
      v = ((unsigned long)a) * 2560 + ((unsigned long)b) * 10;
      units = F("km");
      break;
    case 37:
      v = b;
      break; // oil pressure ?!
    // ADP: FIXME!
    /*case 37: switch(b){
           case 0: sprintf(buf, F("ADP OK (%d,%d)"), a,b); t=String(buf); break;
           case 1: sprintf(buf, F("ADP RUN (%d,%d)"), a,b); t=String(buf); break;
           case 0x10: sprintf(buf, F("ADP ERR (%d,%d)"), a,b); t=String(buf); break;
           default: sprintf(buf, F("ADP (%d,%d)"), a,b); t=String(buf); break;
        }*/
    case 38:
      v = (b - 128) * 0.001 * a;
      units = F("Deg k/w");
      break;
    case 39:
      v = b / 256 * a;
      units = F("mg/h");
      break;
    case 40:
      v = b * 0.1 + (25.5 * a) - 400;
      units = F("A");
      break;
    case 41:
      v = b + a * 255;
      units = F("Ah");
      break;
    case 42:
      v = b * 0.1 + (25.5 * a) - 400;
      units = F("Kw");
      break;
    case 43:
      v = b * 0.1 + (25.5 * a);
      units = F("V");
      break;
    case 44:
      sprintf(buf, "%2d:%2d", a, b);
      t = String(buf);
      break;
    case 45:
      v = 0.1 * a * b / 100;
      units = F(" ");
      break;
    case 46:
      v = (a * b - 3200) * 0.0027;
      units = F("Deg k/w");
      break;
    case 47:
      v = (b - 128) * a;
      units = F("ms");
      break;
    case 48:
      v = b + a * 255;
      units = F(" ");
      break;
    case 49:
      v = (b / 4) * a * 0.1;
      units = F("mg/h");
      break;
    case 50:
      v = (b - 128) / (0.01 * a);
      units = F("mbar");
      break;
    case 51:
      v = ((b - 128) / 255) * a;
      units = F("mg/h");
      break;
    case 52:
      v = b * 0.02 * a - a;
      units = F("Nm");
      break;
    case 53:
      v = (b - 128) * 1.4222 + 0.006 * a;
      units = F("g/s");
      break;
    case 54:
      v = a * 256 + b;
      units = F("count");
      break;
    case 55:
      v = a * b / 200;
      units = F("s");
      break;
    case 56:
      v = a * 256 + b;
      units = F("WSC");
      break;
    case 57:
      v = a * 256 + b + 65536;
      units = F("WSC");
      break;
    case 59:
      v = (a * 256 + b) / 32768;
      units = F("g/s");
      break;
    case 60:
      v = (a * 256 + b) * 0.01;
      units = F("sec");
      break;
    case 62:
      v = 0.256 * a * b;
      units = F("S");
      break;
    case 64:
      v = float(a + b);
      units = F("Ohm");
      break;
    case 65:
      v = 0.01 * a * (b - 127);
      units = F("mm");
      break;
    case 66:
      v = (a * b) / 511.12;
      units = F("V");
      break;
    case 67:
      v = (640 * a) + b * 2.5;
      units = F("Deg");
      break;
    case 68:
      v = (256 * a + b) / 7.365;
      units = F("deg/s");
      break;
    case 69:
      v = (256 * a + b) * 0.3254;
      units = F("Bar");
      break;
    case 70:
      v = (256 * a + b) * 0.192;
      units = F("m/s^2");
      break;
    default:
      sprintf(buf, "%2x, %2x      ", a, b);
      break;
    }

    switch (currAddr)
    {
    case ADR_Engine:
      switch (group)
      {
      case 4:
        switch (idx)
        {
        case 0:
          engineSpeed = v;
          break;
        case 1:
          supplyVoltage = v;
          break; // OK
        case 2:
          coolantTemp = (int)v;
          break; // OK
        case 3:
          intakeAirTemp = (int)v;
          break;
          break;
        }
        break;
      case 5:
        switch (idx)
        {
        case 1:
          engineLoad = (int)v;
          break;
        case 2:
          vehicleSpeed = (int)v;
          break; // OK
          break;
        }
        break;
      case 134:
        switch (idx)
        {
        case 0:
          oilTemp = (int)v;
          break; // or 134-3
          break;
        }
        break;
      }
      break;
    case ADR_Dashboard:
      switch (group)
      {
      case 1:
        switch (idx)
        {
        case 0:
          vehicleSpeed = v;
          break;
        // case 1:
        //   engineSpeed = v;
        //   break;
        case 2:
          oilPressure = v;
          break;
        }
        break;
      case 2:
        switch (idx)
        {
        case 0:
          odometer = v;
          break;
        case 1:
          fuelLevel = v;
          break;
        }
        break;
      case 50:
        switch (idx)
        {
        // case 1:
        //   engineSpeed = v;
        //   break;
        case 2:
          oilTemp = v;
          break;
          // case 3:
          //   coolantTemp = v;
          //   break;
        }
        break;
      }
      break;
    }
    if (units.length() != 0)
    {
      dtostrf(v, 4, 2, buf);
      t = String(buf) + " " + units;
    }
    //    Serial.println(t);

    // lcd.setCursor(0, idx);
    // while (t.length() < 20) t += " ";
    // lcd.print(t);
  }
  sensorCounter++;
  return true;
}

void alarm()
{
  //  if (alarmCounter > 10) return;
  //  tone(pinBuzzer, 1200);
  //  delay(100);
  //  noTone(pinBuzzer);
  //  alarmCounter++;
}
#pragma endregion

void updateDisplay()
{
  Serial.println("___________________________________________");
  Serial.println("RPM: " + String(engineSpeed));
  Serial.println("Coolant temp: " + String(coolantTemp));
  Serial.println("Intake temp: " + String(intakeAirTemp));
  Serial.println("Battery voltage: " + String(supplyVoltage));
  Serial.println("Vehicle speed: " + String(vehicleSpeed));
  Serial.println("___________________________________________");
  Serial.println("Fuel lvl: " + String(fuelLevel));
  Serial.println("Odometer: " + String(odometer));
  Serial.println("OilTemp: " + String(oilTemp));
  Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}
void mapDataToI2C()
{
  if (connected)
  {
    if (currAddr == ADR_Engine)
      slave_data.diagConnectionStatus = 1;
    else if (currAddr == ADR_Dashboard)
      slave_data.diagConnectionStatus = 2;
    else
      slave_data.diagConnectionStatus = 0;

    if (connected == true)
      slave_data.connectionStatus = 1;
    else
      slave_data.connectionStatus = 0;

    slave_data.coolantTemp = coolantTemp;
    slave_data.intakeAirTemp = intakeAirTemp;
    slave_data.oilTemp = oilTemp;
    slave_data.batteryVoltage = int(supplyVoltage * 100);
    slave_data.fuelLevel = fuelLevel;
    slave_data.rpm = engineSpeed;
    slave_data.vehicleSpeed = vehicleSpeed;

    if (engineSpeed > 1)
      slave_data.isEngineWorking = 1;
    else
      slave_data.isEngineWorking = 0;
  }
  // In case of lost connection
  else
  {
    slave_data.diagConnectionStatus = 0;
    slave_data.connectionStatus = 0;
    slave_data.coolantTemp = 0;
    slave_data.intakeAirTemp = 0;
    slave_data.oilTemp = 0;
    slave_data.batteryVoltage = 0;
    slave_data.fuelLevel = 0;
    slave_data.rpm = 0;
    slave_data.vehicleSpeed = 0;
    slave_data.isEngineWorking = 0;
  }
}

void setup()
{
  Serial.begin(9600);
  // Serial.println(F("SETUP"));
  setupI2C();
  // char buff21[21];

  // EEPROM.write(vMaxAddr, 0);  // zero the eeprom value   for vMax
  // vMax = (int)EEPROM.read(vMaxAddr);
  // sprintf(buff21, "SETUP  --- vMax: %3d", vMax);

  pinMode(pinKLineTX, OUTPUT);
  digitalWrite(pinKLineTX, HIGH);

  //  Serial.println(F("START"));
}

// 1 for ECU, 2 for DASH
int currBlock = 1;
int updateLoopsElapsed = 0;

void loop()
{
  // errorTimeout = 0;
  // errorData = 0;
  switch (currBlock)
  {
  case 2:
    Serial.println("Accessing DASH");
    if (currAddr != ADR_Dashboard)
    {
      delay(1000);
      connect(ADR_Dashboard, 10400);
    }
    else
    {
      readSensors(1);
      readSensors(2);
      readSensors(50);
    }
    break;
  case 1:
    Serial.println("Accessing ECU");
    if (currAddr != ADR_Engine)
    {
      /////////////////////////////////////////delay(1000) stable passat ecu connection
      delay(1000);
      connect(ADR_Engine, 10400);
    }
    else
    {
      readSensors(4); // 4-1  Versorgungsspannung[V]  4-2 Kuehlmitteltemp [°C]
      readSensors(5); // 5-1  Motorlast [%]           5-2 Geschwindigkeit [Km/H]
      // readSensors(134); // 134-0 Oeltemp [°C]
    }
    break;
  }

  if (currBlock == 1 && updateLoopsElapsed >= 30)
  {
    currBlock = 2;
    updateLoopsElapsed = 0;
  }
  else if (currBlock == 2 && updateLoopsElapsed >= 5)
  {
    currBlock = 1;
    updateLoopsElapsed = 0;
  }
  else
  {
    updateLoopsElapsed++;
  }

  updateDisplay();
  mapDataToI2C();
}
