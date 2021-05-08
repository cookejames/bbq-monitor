#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <display.h>
#include <secrets.h>
#include <awsiot.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <controller.h>

#define DISPLAY_WIDTH GxEPD_HEIGHT
#define DISPLAY_HEIGHT GxEPD_WIDTH

GxIO_Class Display::io(SPI, EPD_CS, EPD_DC, EPD_RSET);
GxEPD_Class Display::display(io, EPD_RSET, EPD_BUSY);
bool Display::hasUpdates = false;
bool Display::hasTemperatureUpdate = false;
bool Display::wifiStatus = false;
bool Display::bleStatus = false;
bool Display::iotStatus = false;
unsigned long Display::lastUpdateTime = 0;
unsigned long Display::lastTemperatureUpdateTime = 0;
unsigned long Display::lastBatteryUpdate = 0;
uint16_t Display::currentTemperature = 0;
U8G2_FOR_ADAFRUIT_GFX Display::u8g2;
const uint8_t *Display::FONT_6_PT = u8g2_font_profont10_mf;
const uint8_t *Display::FONT_9_PT = u8g2_font_profont15_mf;
const uint8_t *Display::FONT_42_PT = u8g2_font_logisoso42_tr;
const uint8_t *Display::FONT_58_PT = u8g2_font_logisoso58_tr;

void Display::init()
{
  display.init();
  u8g2.begin(display);
  u8g2.setForegroundColor(GxEPD_BLACK);
  u8g2.setBackgroundColor(GxEPD_WHITE);
  u8g2.setFontMode(0);

  display.eraseDisplay();
  display.setRotation(1);
  setStatus(false, false, false);
  drawBattery();
  drawLabels();
  hasUpdates = false;
  // drawGrid();
  display.update();
}

void Display::check()
{
  if (millis() > lastBatteryUpdate + DISPLAY_MIN_TIME_BETWEEN_BATTERY_UPDATES)
  {
    lastBatteryUpdate = millis();
    drawBattery();
  }

  if (!hasUpdates && !hasTemperatureUpdate)
  {
    return;
  }

  bool shouldUpdate = false;
  if (hasUpdates && millis() > (lastUpdateTime + DISPLAY_MIN_TIME_BETWEEN_UPDATES))
  {
    hasUpdates = false;
    lastUpdateTime = millis();
    shouldUpdate = true;
  }

  // Only update the display if the temperature has changed and enough time has elapsed
  // The temeprature changes frequently and the display blinking is annoying
  if (hasTemperatureUpdate && millis() > (lastTemperatureUpdateTime + DISPLAY_MIN_TIME_BETWEEN_TEMPERATURE_UPDATES))
  {
    hasTemperatureUpdate = false;
    lastTemperatureUpdateTime = millis();
    shouldUpdate = true;
  }

  if (shouldUpdate)
  {
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

  // Clear the area
  display.fillRect(DISPLAY_WIDTH - (ICON_SIZE_BIG * 6), 0, ICON_SIZE_BIG * 6, ICON_SIZE_BIG, GxEPD_WHITE);

  // Draw the icons
  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 2), ICON_SIZE_BIG, ICON_CLOUD);
  u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 4), ICON_SIZE_BIG, ICON_BT);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 6), ICON_SIZE_BIG, ICON_WIFI);

  // Draw the status ticks/crosses
  u8g2.setFont(u8g2_font_open_iconic_check_1x_t);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 1), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, iot ? ICON_TICK : ICON_CROSS);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 3), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, ble ? ICON_TICK : ICON_CROSS);
  u8g2.drawGlyph(DISPLAY_WIDTH - (ICON_SIZE_BIG * 5), ICON_SIZE_SMALL + ICON_SIZE_SMALL / 2, wifi ? ICON_TICK : ICON_CROSS);

  hasUpdates = true;
}

void Display::setIpAddress(const char *ipAddress)
{
  u8g2.setFont(FONT_6_PT);
  u8g2.setCursor(0, 8);
  u8g2.print(THINGNAME);
  u8g2.setCursor(0, 16);
  u8g2.print(ipAddress);

  hasUpdates = true;
}

void Display::setTemperature(uint16_t temperature)
{
  if (temperature == currentTemperature)
  {
    return;
  }
  currentTemperature = temperature;
  display.fillRect(10, 42, 120, 58, GxEPD_WHITE);
  u8g2.setFont(FONT_58_PT);
  u8g2.setCursor(10, 100);
  u8g2.print(temperature);
  display.updateWindow(10, 42, 120, 58, true);

  hasTemperatureUpdate = true;
}

void Display::drawSetpoint(int x, int y, int16_t temperature)
{
  display.fillRect(x, y - 58, DISPLAY_WIDTH - x, 58, GxEPD_WHITE);

  u8g2.setFont(FONT_9_PT);
  u8g2.setCursor(x, y);
  u8g2.setFont(u8g2_font_logisoso58_tr);
  u8g2.print(temperature);
}

void Display::drawSetpoint(int x, int y, const char *state)
{
  display.fillRect(x, y - 58, DISPLAY_WIDTH - x, 58, GxEPD_WHITE);

  u8g2.setFont(FONT_9_PT);
  u8g2.setCursor(x + 20, y);
  u8g2.setFont(FONT_58_PT);
  u8g2.print(state);
}

void Display::setSetpoint(int16_t temperature, bool partialUpdate)
{
  int x = 130;
  int y = 100;

  if (temperature == SETPOINT_MANUAL_OVERRIDE)
  {
    drawSetpoint(x, y, "M");
  }
  else
  {
    drawSetpoint(x, y, temperature);
  }

  if (partialUpdate)
  {
    display.updateWindow(x, y - 58, DISPLAY_WIDTH - x, 58, true);
  }
  else
  {
    hasUpdates = true;
  }
}

void Display::setSetpoint(int16_t temperature)
{
  setSetpoint(temperature, false);
}

void Display::setStartupMode()
{
  int x = 130;
  int y = 100;

  drawSetpoint(x, y, "S");
  hasUpdates = true;
}

void Display::setLidOpenMode()
{
  int x = 130;
  int y = 100;

  drawSetpoint(x, y, "L");
  hasUpdates = true;
}

void Display::drawBattery()
{
  int x = DISPLAY_WIDTH - (16 * 8);
  int y = 16;
  float minLiPoV = 3.4;
  float maxLiPoV = 4.2;
  float percentage = 1.0;
  // analog value = Vbat / 2
  int voltageRaw = 0;
  for (int i = 0; i < 10; i++)
  {
    voltageRaw += analogRead(35);
  }
  voltageRaw /= 10;
  // voltage = divider * V_ref / Max_Analog_Value
  float voltage = 2.205 * 3.27 * voltageRaw / 4096.0;
  if (voltage > 1)
  { // Only display if there is a valid reading
    percentage = (voltage - minLiPoV) / (maxLiPoV - minLiPoV);
    if (voltage >= maxLiPoV)
      percentage = 1;
    if (voltage <= minLiPoV)
      percentage = 0;

    Log.trace("Battery raw voltage: %FV, calculated voltage: %FV, percentage %F", voltageRaw, voltage, percentage);
    int bodyHeight = 14;
    int capHeight = 2;
    int width = 7;
    display.fillRect(x, y - bodyHeight - capHeight, width, bodyHeight + capHeight, GxEPD_WHITE);
    // body
    display.fillRect(x, y - bodyHeight, width, bodyHeight, GxEPD_BLACK);
    // cap
    display.fillRect(x + 1, y - bodyHeight - capHeight, width - 2, capHeight, GxEPD_BLACK);
    // percent used
    display.fillRect(x + 1, y - bodyHeight + 1, width - 2, ((double)(bodyHeight - 2) * (1 - percentage)), GxEPD_WHITE);

    // Percentage text
    u8g2.setFont(FONT_6_PT);
    // Clear
    display.fillRect(x + width + 2, 0, 8, 16, GxEPD_WHITE);
    if (percentage == 1)
    {
      u8g2.setCursor(x + width + 2, y - 8);
      u8g2.print((int)(percentage * 100));
      u8g2.setCursor(x + width + 7, y);
      u8g2.print("%");
    }
    else
    {
      u8g2.setCursor(x + width + 2, y - 4);
      char buffer[3];
      sprintf(buffer, "%d%%", (int)(percentage * 100));
      u8g2.print(buffer);
    }

    updateBatteryShadow(voltage, percentage);
    hasUpdates = true;
  }
}

void Display::updateBatteryShadow(float voltage, float percentage)
{
  const int capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["voltage"] = roundf(voltage * 100) / 100;
  reported["percentage"] = roundf(percentage * 100) / 100;
  char output[128];
  serializeJson(doc, output);
  AwsIot::publishToShadow("battery", "update", output);
}

void Display::setTunings(double Kp, double Ki, double Kd)
{
  u8g2.setFont(FONT_6_PT);
  u8g2.setCursor(0, DISPLAY_HEIGHT - 8);
  char buffer[DISPLAY_WIDTH];
  sprintf(buffer, "Kp: %3.2f - Ki: %3.3f - Kd: %3.2f", Kp, Ki, Kd);
  u8g2.print(buffer);

  hasUpdates = true;
}

void Display::drawLabels()
{
  // Top line
  display.drawFastHLine(0, 18, DISPLAY_WIDTH, GxEPD_BLACK);

  // Bottom line
  display.drawFastHLine(0, DISPLAY_HEIGHT - 18, DISPLAY_WIDTH, GxEPD_BLACK);

  // Temperature label
  u8g2.setFont(FONT_9_PT);
  u8g2.setCursor(20, 35);
  u8g2.print("temperature");

  // Setpoint labels
  u8g2.setCursor(150, 35);
  u8g2.print("setpoint");

  hasUpdates = true;
}

void Display::setPidOutput(uint8_t output)
{
  int16_t SIZE = 16;
  int16_t ICON_FAN = 66;
  uint16_t x = DISPLAY_WIDTH - (SIZE * 2);
  uint16_t y = DISPLAY_HEIGHT;

  // Clear
  display.fillRect(x, y - SIZE, SIZE * 2, SIZE, GxEPD_WHITE);

  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  u8g2.drawGlyph(x, y - 7, ICON_FAN);

  u8g2.setFont(FONT_6_PT);
  u8g2.setCursor(x + (SIZE / 2) + 2, y - 8);
  char buffer[3];
  sprintf(buffer, "%d%%", output);
  u8g2.print(buffer);

  display.updateWindow(x, y - SIZE, SIZE * 2, SIZE, true);
}