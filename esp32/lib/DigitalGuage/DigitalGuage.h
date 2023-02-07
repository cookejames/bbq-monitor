#ifndef DigitalGuage_h
#define DigitalGuage_h
#include <TFT_eSPI.h>

class DigitalGuage
{
private:
    TFT_eSPI *tft;
    // The centre point of the guage
    uint16_t xPos = 20;
    uint16_t yPos = 20;
    // The radius of the guage
    uint16_t radius = 20;
    // The last angle that was set
    uint16_t lastAngle = 30;
    // The opening angle at the bottom of the guage
    uint16_t openAngle = 30;
    // The break points for different colours
    uint16_t break1 = 25;
    uint16_t break2 = 50;
    uint16_t break3 = 75;
    uint16_t breakAngle1;
    uint16_t breakAngle2;
    uint16_t breakAngle3;
    // The minimum value of the guage
    int minVal = 0;
    // The maximum value of the guage
    int maxVal = 100;
    // The first colour of the guage
    int colour0 = TFT_GREEN;
    // The second colour of the guage
    int colour1 = TFT_YELLOW;
    // The third colour of the guage
    int colour2 = TFT_ORANGE;
    // The fourth colour of the guage
    int colour3 = TFT_RED;
    // Has the guage been initialised
    bool hasInit = false;
    // The inner colour of the guage
    int innerColour = 0x18E3;
    // The border colour of the guage
    int borderColour = TFT_SILVER;
    // The background colour of the guage
    int backgroundColour = TFT_BLACK;
    bool displayValue = false;
    int textColour = TFT_YELLOW;
    const char* units = "";
    uint16_t mapAngle(uint16_t);

public:
    DigitalGuage(TFT_eSPI *);
    DigitalGuage(TFT_eSPI *, int, int);
    void init(uint16_t, uint16_t, uint16_t);
    void init();
    void setVal(uint16_t);
    void setInnerColour(int);
    void setBorderColour(int);
    void setBackgroundColour(int);
    void setTextColour(int);
    void setDisplayValue(bool, const char*);
    void setDisplayValue(bool);
};
#endif