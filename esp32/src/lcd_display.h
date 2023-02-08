#ifndef lcd_display_h
#define lcd_display_h
#include <config.h>
#ifdef HAS_LCD_DISPLAY
#include <FS.h>
#include <SPIFFS.h>
#include <Arduino.h>
#ifdef USE_HQ_IMAGES
#include <PNGdec.h>
#endif
#include <DigitalGuage.h>

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
  static void setLidOpenMode();
  static void check();

private:
  static int16_t imageXpos;
  static int16_t imageYpos;
  static bool hasUpdates;
  static bool wifiStatus;
  static bool bleStatus;
  static bool iotStatus;
  static bool startupMode;
  static bool lidOpenMode;
  static char ipAddress[];
  static int16_t setpoint;
  static int16_t currentTemperature;
  static void showVoltage();
  static void drawStatus();
  static void drawTemperature();
  static void clearTemperature();
  static void drawGrid();
  static DigitalGuage setpointGuage;
  static DigitalGuage temperatureGuage;
  static TFT_eSPI tft;
  #ifdef USE_HQ_IMAGES
  static PNG png;
  static void pngDrawCb(PNGDRAW *);
  static void pngDraw(const unsigned char *, int16_t, int16_t, int16_t);
  #endif
};
#endif

#endif