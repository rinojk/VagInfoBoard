#ifndef DisplayManager_h
#define DisplayManager_h

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>

#include "Arduino.h"
#include "Structs.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
class DisplayManager{
  public:
    DisplayManager();
    ~DisplayManager();
    void setupDisplay();
    void displayInfo(String text1, String text2, String text3, String mainText, bool warning);
    void renderScreen(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData);
    void renderScreen1(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData);
    void renderScreen2(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData);
    void renderScreen3(SLAVE_DATA &slave_data, BoardInfoSystemData &infoBoardData);
};

#endif