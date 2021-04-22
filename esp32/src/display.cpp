#include <display.h>

#define EPD_BUSY 4 // to EPD BUSY
#define EPD_CS 5   // to EPD CS
#define EPD_RST 16 // to EPD RST
#define EPD_DC 17  // to EPD DC

Display::Display()  : io(SPI, EPD_CS, EPD_DC, EPD_RST), display(io, EPD_RST, EPD_BUSY)
{
  display.init();
  display.eraseDisplay(); // display.update();

  display.setTextColor(GxEPD_BLACK);
  display.setRotation(1);
  // setStatus(false, false, false);

  // display.update();
}

void Display::setStatus(bool wifi, bool iot, bool ble)
{
  uint8_t lineHeight = 20;
  display.setCursor(0, 0);
  display.setBackgroundColor(GxEPD_WHITE);
  display.fillRect(0, 0, GxEPD_WIDTH, lineHeight, GxEPD_WHITE);
  // display.updateWindow(0, 0, GxEPD_WIDTH, lineHeight, true);
  display.print("WiFi");
  addCloud(40, 10, 2, 2);
  display.setCursor(75, 0);
  display.print("IoT");
  addCloud(110, 10, 2, 2);

  display.setCursor(150, 0);
  display.print("BLE");
  addCloud(190, 10, 2, 2);
  display.updateWindow(0, 0, GxEPD_WIDTH, lineHeight, true);
}

void Display::addCloud(int x, int y, int scale, int linesize)
{
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                              // Left most circle
  display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                              // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);                    // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK);       // Right middle upper circle
  display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);                                                   // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);                                                   // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE);                                         // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE);                            // Right middle upper circle
  display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, GxEPD_WHITE); // Upper and lower lines
}