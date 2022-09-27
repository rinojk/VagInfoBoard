#ifndef EEPROMmanager_h
#define EEPROMmanager_h

#include <Arduino.h>
#include <EEPROM.h>
#include "Structs.h"

class EEPROMmanager{
    public:
        byte eepromAddress = 0;
        EEPROMmanager();
        ~EEPROMmanager();
        void setupEEPROMData(BoardInfoSystemData &infoBoardData);
        void updateData(BoardInfoSystemData &infoBoardData);
};

#endif
