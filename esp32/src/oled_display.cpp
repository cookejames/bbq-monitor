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

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
bool Display::wifiStatus = false;
bool Display::bleStatus = false;
bool Display::iotStatus = false;
bool Display::hasUpdates = false;
int vref = 1100;
#define ADC_PIN         34

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
    if (!hasUpdates) return;
    hasUpdates = false;

    Log.trace("Updating display");
    tft.setCursor(0,0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TL_DATUM);
    tft.printf("BT: %s Wifi: %s IoT: %s", bleStatus ? "x" : "-", wifiStatus ? "x" : "-", iotStatus ? "x" : "-");
}

void Display::setStatus(bool wifi, bool iot, bool ble)
{
    if (wifiStatus == wifi && iotStatus == iot && bleStatus == ble) return;

    wifiStatus = wifi;
    iotStatus = iot;
    bleStatus = ble;
    hasUpdates = true;
    Display::check();
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

void Display::showVoltage()
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "Voltage :" + String(battery_voltage) + "V";
        Serial.println(voltage);
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
    }
}
#endif