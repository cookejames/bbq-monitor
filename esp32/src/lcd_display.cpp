#include <config.h>
#ifdef HAS_LCD_DISPLAY
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <lcd_display.h>
#include <secrets.h>
#include <awsiot.h>
#include <controller.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <PNGdec.h>
#include <images.h>

bool Display::wifiStatus = false;
bool Display::bleStatus = false;
bool Display::iotStatus = false;
bool Display::hasUpdates = false;
bool Display::startupMode = false;
bool Display::lidOpenMode = false;
char Display::ipAddress[] = "000.000.000.000";
int16_t Display::setpoint = 0;
int16_t Display::currentTemperature = 0;
int16_t Display::imageXpos = 0;
int16_t Display::imageYpos = 0;
PNG Display::png;
TFT_eSPI Display::tft = TFT_eSPI();
DigitalGuage Display::setpointGuage = DigitalGuage(&tft, 0, 400);
DigitalGuage Display::temperatureGuage = DigitalGuage(&tft, 0, 400);

void Display::init()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    hasUpdates = true;
    temperatureGuage.init(40, 65, 50);
    temperatureGuage.setDisplayValue(true, "C");
    setpointGuage.init(140, 65, 50);
    setpointGuage.setDisplayValue(true, "C");
    check();
}

void Display::check()
{
    if (!hasUpdates)
        return;
    hasUpdates = false;

    Log.trace("Updating display");
    tft.setCursor(0, 0);

    Display::drawStatus();
    Display::drawTemperature();
    // Display::drawGrid();
}

void Display::setStatus(bool wifi, bool iot, bool ble)
{
    if (wifiStatus == wifi && iotStatus == iot && bleStatus == ble)
        return;

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
    hasUpdates = true;
    startupMode = false;
    lidOpenMode = false;
    currentTemperature = temperature;
    Display::check();
}

void Display::setSetpoint(int16_t temperature, bool partialUpdate)
{
    setSetpoint(temperature);
}

void Display::setSetpoint(int16_t temperature)
{
    if (startupMode || lidOpenMode)
    {
        clearTemperature();
        temperatureGuage.init();
        setpointGuage.init();
    }
    hasUpdates = true;
    startupMode = false;
    lidOpenMode = false;
    setpoint = temperature;
    Display::check();
}

void Display::setStartupMode()
{
    if (startupMode)
        return;
    hasUpdates = true;
    startupMode = true;
    lidOpenMode = false;
    clearTemperature();
    Display::check();
}

void Display::setLidOpenMode()
{
    if (lidOpenMode)
        return;
    hasUpdates = true;
    lidOpenMode = true;
    startupMode = false;
    clearTemperature();
    Display::check();
}

void Display::setTunings(double Kp, double Ki, double Kd)
{
}

void Display::setPidOutput(uint8_t output)
{
}

/*****   PRIVATE    *****/
void Display::pngDrawCb(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[TFT_WIDTH];        // Line buffer for rendering
    uint8_t maskBuffer[1 + TFT_WIDTH / 8]; // Mask buffer

    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (png.getAlphaMask(pDraw, maskBuffer, 255))
    {
        tft.pushMaskedImage(imageXpos, imageYpos + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
    }
}

void Display::pngDraw(const unsigned char *image, int16_t size, int16_t x, int16_t y)
{
    imageXpos = x;
    imageYpos = y;
    int16_t rc = png.openFLASH((uint8_t *)image, size, pngDrawCb);

    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();
    tft.endWrite();
}

void Display::drawStatus()
{
    tft.setTextDatum(TC_DATUM);
    tft.setCursor(0, 32);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    uint8_t h = tft.fontHeight();
    // Clear the status bar
    tft.fillRect(0, 0, TFT_HEIGHT, 40, TFT_BLACK);

    static const unsigned char *ok = icon_tick;
    uint16_t sOk = sizeof(icon_tick);
    static const unsigned char *nOk = icon_cross;
    uint16_t snOk = sizeof(icon_cross);

    pngDraw(icon_wifi, sizeof(icon_wifi), 0, 0);
    pngDraw(wifiStatus ? ok : nOk, wifiStatus ? sOk : snOk, 40, 0);
    pngDraw(icon_cloud_connection, sizeof(icon_cloud_connection), 80, 0);
    pngDraw(iotStatus ? ok : nOk, iotStatus ? sOk : snOk, 120, 0);
#ifdef USE_IBBQ
    pngDraw(icon_bluetooth, sizeof(icon_bluetooth), 160, 0);
    pngDraw(bleStatus ? ok : nOk, bleStatus ? sOk : snOk, 200, 0);
#endif

    tft.setTextWrap(false, false);
    tft.print(THINGNAME);
    tft.print(" - ");
    tft.println(ipAddress);
}

void Display::clearTemperature()
{
    tft.fillRect(0, 40, TFT_HEIGHT, TFT_WIDTH - 40, TFT_BLACK);
}

void Display::drawTemperature()
{
    uint8_t padding = 50;
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(3);
    if (startupMode)
    {
        //
        pngDraw(icon_fan_80, sizeof(icon_fan_80), 0, padding);
        uint16_t h = tft.fontHeight();
        tft.drawString("Startup", 90, padding + 10);
        tft.drawString("mode", 90, padding + 10 + h);
    }
    else if (lidOpenMode)
    {
        pngDraw(icon_grill_80, sizeof(icon_grill_80), 0, padding);
        uint16_t h = tft.fontHeight();
        tft.drawString("Lid", 90, padding + 10);
        tft.drawString("open", 90, padding + 10 + h);
    }
    else
    {
        tft.setTextColor(TFT_YELLOW);
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(1);
        tft.drawString("Temperature", 65, 50);
        tft.drawString("Setpoint", 165, 50);
        temperatureGuage.setVal(currentTemperature);
        setpointGuage.setVal(setpoint);
    }
}

void Display::drawGrid()
{
    for (uint16_t i = 10; i < TFT_HEIGHT; i += 10)
    {
        tft.drawLine(i, 0, i, TFT_WIDTH, TFT_LIGHTGREY);
    }
    for (uint16_t i = 10; i < TFT_WIDTH; i += 10)
    {
        tft.drawLine(0, i, TFT_HEIGHT, i, TFT_LIGHTGREY);
    }
}
#endif