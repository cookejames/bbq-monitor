#ifndef oled_display_h
#define oled_display_h
  #include <config.h>
  #ifdef BOARD_TTGO_DISPLAY
  #include <FS.h>
  #include <SPIFFS.h>
  #include <Arduino.h>

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
    static bool hasUpdates;
    static bool wifiStatus;
    static bool bleStatus;
    static bool iotStatus;
    static void showVoltage();
  };
  #endif

#endif