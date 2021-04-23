#ifndef display_h
#define display_h
#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <config.h>
#include <boards.h>

class Display
{
public:
  Display();
  void setStatus(bool, bool, bool);
  void check();

private:
  GxIO_Class io;
  GxEPD_Class display;
  U8G2_FOR_ADAFRUIT_GFX u8g2;
  void drawGrid();
  void writeLine(int, const GFXfont *);
  void writeLine(int, const uint8_t *);
  void writeLine(int);
  bool hasUpdates = true;
  unsigned long lastUpdateTime = 0;
  bool wifiStatus;
  bool bleStatus;
  bool iotStatus;
};

#endif