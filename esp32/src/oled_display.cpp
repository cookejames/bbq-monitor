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
#include <PNGdec.h>
#include <images.h>

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
bool Display::wifiStatus = false;
bool Display::bleStatus = false;
bool Display::iotStatus = false;
bool Display::hasUpdates = false;
char Display::ipAddress[] = "000.000.000.000";
int16_t Display::imageXpos = 0;
int16_t Display::imageYpos = 0;
PNG Display::png;

int vref = 1100;
#define ADC_PIN         34


void Display::init()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(0,0);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("BBQ Monitor", TFT_HEIGHT/2, TFT_WIDTH/2);
    int16_t h = tft.fontHeight();
    tft.setTextSize(1);
    tft.drawString("Loading...", TFT_HEIGHT/2, TFT_WIDTH/2 + h);
}

void Display::check()
{
    if (!hasUpdates) return;
    hasUpdates = false;

    Log.trace("Updating display");
    tft.setCursor(0,0);
    tft.fillScreen(TFT_BLACK);

    Display::drawStatus();
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

void Display::setIpAddress(const char *_ipAddress)
{
    strcpy(ipAddress, _ipAddress);
    hasUpdates = true;
    Display::check();
}

void Display::setTemperature(uint16_t temperature)
{

}

void Display::setSetpoint(int16_t temperature, bool partialUpdate)
{
    setSetpoint(temperature);
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

/*****   PRIVATE    *****/
void Display::pngDrawCb(PNGDRAW *pDraw) {
  uint16_t lineBuffer[TFT_WIDTH];          // Line buffer for rendering
  uint8_t  maskBuffer[1 + TFT_WIDTH / 8];  // Mask buffer

  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

  if (png.getAlphaMask(pDraw, maskBuffer, 255)) {
    tft.pushMaskedImage(imageXpos, imageYpos + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
  }
}

void Display::pngDraw(const unsigned char *image, int16_t size, int16_t x, int16_t y) {
    imageXpos = x;
    imageYpos = y;
    int16_t rc = png.openFLASH((uint8_t*)image, size, pngDrawCb);

    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();
    tft.endWrite();
}

void Display::drawStatus() {
    static const unsigned char* ok = icon_tick;
    uint16_t sOk = sizeof(icon_tick);
    static const unsigned char* nOk = icon_cross;
    uint16_t snOk = sizeof(icon_cross);


    pngDraw(icon_wifi, sizeof(icon_wifi), 0, 0);
    pngDraw(wifiStatus ? ok : nOk, wifiStatus ? sOk : snOk, 40, 0);
    pngDraw(icon_cloud_connection, sizeof(icon_cloud_connection), 80, 0);
    pngDraw(iotStatus ? ok : nOk, iotStatus ? sOk : snOk, 120, 0);
    #ifdef USE_IBBQ
    pngDraw(icon_bluetooth, sizeof(icon_bluetooth), 160, 0);
    pngDraw(bleStatus ? ok : nOk, bleStatus ? sOk : snOk, 200, 0);
    #endif

    tft.setTextDatum(TL_DATUM);
    tft.setCursor(0,32);
    tft.setTextSize(1);
    tft.print(THINGNAME);
    tft.print(" - ");
    tft.println(ipAddress);
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