#include <config.h>
#ifdef BOARD_TTGO_DISPLAY
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <oled_display.h>
#include <secrets.h>
#include <awsiot.h>
#include <controller.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TTGO_T_Display.h>

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);

void Display::init()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.println("BBQ Monitor");
    tft.setTextSize(1);
    tft.println("Loading...");
    tft.setTextSize(1);
}

void Display::check()
{

}

void Display::setStatus(bool wifi, bool iot, bool ble)
{
  
}

void Display::setIpAddress(const char *ipAddress)
{

}

void Display::setTemperature(uint16_t temperature)
{

}

void Display::setSetpoint(int16_t temperature, bool partialUpdate)
{

}

void Display::setSetpoint(int16_t temperature)
{

}

void Display::setStartupMode()
{

}

void Display::setLidOpenMode()
{

}

void Display::setTunings(double Kp, double Ki, double Kd)
{

}

void Display::setPidOutput(uint8_t output)
{

}
#endif