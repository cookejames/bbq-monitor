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
  static void init();
  static void setStatus(bool, bool, bool);
  static void setIpAddress(const char *);
  static void setTemperature(uint16_t);
  static void setSetpoint(int16_t);
  static void setSetpoint(int16_t, bool);
  static void setTunings(double, double, double);
  static void setPidOutput(uint8_t);
  static void setStartupMode();
  static void check();

private:
  static GxIO_Class io;
  static GxEPD_Class display;
  static U8G2_FOR_ADAFRUIT_GFX u8g2;
  static void drawGrid();
  static void drawBattery();
  static void drawLabels();
  static void drawSetpoint(int, int, int16_t);
  static void drawSetpoint(int, int, const char *);
  static void updateBatteryShadow(float, float);
  static bool hasUpdates;
  static bool hasTemperatureUpdate;
  static unsigned long lastUpdateTime;
  static unsigned long lastTemperatureUpdateTime;
  static unsigned long lastBatteryUpdate;
  static bool wifiStatus;
  static bool bleStatus;
  static bool iotStatus;
  static uint16_t currentTemperature;
  static const uint8_t *FONT_6_PT;
  static const uint8_t *FONT_9_PT;
  static const uint8_t *FONT_42_PT;
  static const uint8_t *FONT_58_PT;
};

#endif