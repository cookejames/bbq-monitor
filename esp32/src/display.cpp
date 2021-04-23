#include <ArduinoLog.h>
#include <display.h>
#include <U8g2_for_Adafruit_GFX.h>

#define DISPLAY_WIDTH GxEPD_HEIGHT
#define DISPLAY_HEIGHT GxEPD_WIDTH

Display::Display() : io(SPI, EPD_CS, EPD_DC, EPD_RSET), display(io, EPD_RSET, EPD_BUSY)
{
  display.init();
  u8g2.begin(display);
  u8g2.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color

  // display.eraseDisplay();
  display.fillScreen(GxEPD_WHITE);
  display.setRotation(1);
  setStatus(false, false, false);

  // drawGrid();
}

void Display::check()
{
  if (hasUpdates && millis() > (lastUpdateTime + DISPLAY_MIN_TIME_BETWEEN_UPDATES))
  {
    hasUpdates = false;
    lastUpdateTime = millis();
    display.update();
  }
}

void Display::drawGrid()
{
  for (int i = 10; i < DISPLAY_WIDTH; i += 10)
  {
    display.drawLine(i, 0, i, DISPLAY_HEIGHT, GxEPD_BLACK);
  }
  for (int i = 10; i < DISPLAY_HEIGHT; i += 10)
  {
    display.drawLine(0, i, DISPLAY_WIDTH, i, GxEPD_BLACK);
  }
  hasUpdates = true;
}

void Display::setStatus(bool wifi, bool iot, bool ble)
{
  if (wifi == wifiStatus && iot == iotStatus && ble == bleStatus)
  {
    return;
  }
  wifiStatus = wifi;
  bleStatus = ble;
  iotStatus = iot;

  int16_t ICON_SIZE_SMALL = 8;
  int16_t ICON_SIZE_BIG = 16;
  int16_t ICON_TICK = 64;
  int16_t ICON_CROSS = 68;
  int16_t ICON_CLOUD = 64;
  int16_t ICON_WIFI = 80;
  int16_t ICON_BT = 74;

  display.drawFastHLine(0, ICON_SIZE_BIG + 2, DISPLAY_WIDTH, GxEPD_BLACK);

  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 2), ICON_SIZE_BIG, ICON_CLOUD);
  u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 4), ICON_SIZE_BIG, ICON_BT);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 6), ICON_SIZE_BIG, ICON_WIFI);

  u8g2.setFont(u8g2_font_open_iconic_check_1x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 1), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, iot ? ICON_TICK : ICON_CROSS);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 3), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, ble ? ICON_TICK : ICON_CROSS);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 5), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, wifi ? ICON_TICK : ICON_CROSS);

  hasUpdates = true;
}