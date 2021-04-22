// GxEPD_MinimumExample by Jean-Marc Zingg

#include <GxEPD.h>

// select the display class to use, only one, copy from GxEPD_Example
#include <GxDEPG0213BN/GxDEPG0213BN.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#define EPD_BUSY 4  // to EPD BUSY
#define EPD_CS 5    // to EPD CS
#define EPD_RST 16  // to EPD RST
#define EPD_DC 17   // to EPD DC
#define EPD_SCK 18  // to EPD CLK
#define EPD_MISO -1 // MISO is not used, as no data from display
#define EPD_MOSI 23 // to EPD DIN

#define SDCARD_SS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RST);
GxEPD_Class display(io, EPD_RST, EPD_BUSY);

void drawHelloWorld()
{
  display.setTextColor(GxEPD_BLACK);
  display.print("Hello World!");
}

void setup()
{
  display.init();
  display.eraseDisplay();
  // comment out next line to have no or minimal Adafruit_GFX code
  display.drawPaged(drawHelloWorld); // version for AVR using paged drawing, works also on other processors
}

void loop(){};