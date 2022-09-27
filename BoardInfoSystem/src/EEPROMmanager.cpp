#include <EEPROMmanager.h>

EEPROMmanager::EEPROMmanager()
{
}

EEPROMmanager::~EEPROMmanager()
{
}

void EEPROMmanager::setupEEPROMData(BoardInfoSystemData &infoBoardData)
{
    EEPROM.begin(255);
    EEPROM.get(0, infoBoardData);
    Serial.println(infoBoardData.initOdometerValue);
}
void EEPROMmanager::updateData(BoardInfoSystemData &infoBoardData)
{
    Serial.println("eeprom write");
    EEPROM.put(0, infoBoardData);
    EEPROM.commit();
}
