#ifndef display_h
#define display_h
#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

class Display
{
public:
  Display();
  void setStatus(bool, bool, bool);

private:
  GxIO_Class io;
  GxEPD_Class display;
  void addCloud(int, int, int, int);
};

#endif