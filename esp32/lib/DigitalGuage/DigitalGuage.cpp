#include <DigitalGuage.h>
#include <TFT_eSPI.h>

DigitalGuage::DigitalGuage(TFT_eSPI *_tft)
{
    tft = _tft;
}

DigitalGuage::DigitalGuage(TFT_eSPI *_tft, int _minVal, int _maxVal)
{
    tft = _tft;

    // Recalculate the break points
    int range = _maxVal - _minVal;
    break1 = range * (float)break1 / maxVal;
    break2 = range * (float)break2 / maxVal;
    break3 = range * (float)break3 / maxVal;
    minVal = _minVal;
    maxVal = _maxVal;
}

void DigitalGuage::init(uint16_t x, uint16_t y, uint16_t diameter)
{
    radius = diameter / 2;
    xPos = x + radius;
    yPos = y + radius;
    init();
}

void DigitalGuage::init()
{
    hasInit = true;
    lastAngle = openAngle;
    tft->fillCircle(xPos, yPos, radius, innerColour);
    tft->drawSmoothCircle(xPos, yPos, radius, borderColour, innerColour);
    uint16_t tmp = radius - 3;
    tft->drawArc(xPos, yPos, tmp, tmp - tmp / 5, openAngle, 360 - openAngle, backgroundColour, innerColour);

    breakAngle1 = mapAngle(break1);
    breakAngle2 = mapAngle(break2);
    breakAngle3 = mapAngle(break3);
}

uint16_t DigitalGuage::mapAngle(uint16_t angle)
{
    return map(angle, minVal, maxVal, openAngle, 360 - openAngle);
}

void DigitalGuage::setInnerColour(int c)
{
    innerColour = c;
}

void DigitalGuage::setBorderColour(int c)
{
    borderColour = c;
}

void DigitalGuage::setBackgroundColour(int c)
{
    backgroundColour = c;
}

void DigitalGuage::setTextColour(int c)
{
    textColour = c;
}

void DigitalGuage::setDisplayValue(bool d, const char *u)
{
    units = u;
    setDisplayValue(d);
}

void DigitalGuage::setDisplayValue(bool d)
{
    displayValue = d;
}

void DigitalGuage::setVal(uint16_t val)
{
    if (!hasInit)
        init();

    // Map the value to an angle
    uint16_t _val = val > maxVal ? maxVal : val;
    _val = val < minVal ? minVal : val;
    int valAngle = mapAngle(_val);

    if (lastAngle == valAngle)
        return;

    uint16_t r = radius;
    r -= 3;

    // Allocate a value to the arc thickness dependant of radius
    uint8_t thickness = r / 5;
    if (r < 25)
        thickness = r / 3;

    if (valAngle < lastAngle)
    {
        tft->drawArc(xPos, yPos, r, r - thickness, valAngle, lastAngle, backgroundColour, innerColour);
    }
    else
    {

        if (valAngle < breakAngle1)
        {
            tft->drawArc(xPos, yPos, r, r - thickness, openAngle, valAngle, colour0, backgroundColour);
        }
        else
        {
            tft->drawArc(xPos, yPos, r, r - thickness, openAngle, breakAngle1, colour0, backgroundColour);
        }

        if (valAngle >= breakAngle1 && valAngle < breakAngle2)
        {
            tft->drawArc(xPos, yPos, r, r - thickness, breakAngle1, valAngle, colour1, backgroundColour);
        }
        else if (valAngle >= breakAngle2)
        {
            tft->drawArc(xPos, yPos, r, r - thickness, breakAngle1, breakAngle2, colour1, backgroundColour);
        }

        if (valAngle >= breakAngle2 && valAngle < breakAngle3)
        {
            tft->drawArc(xPos, yPos, r, r - thickness, breakAngle2, valAngle, colour2, backgroundColour);
        }
        else if (valAngle >= breakAngle3)
        {
            tft->drawArc(xPos, yPos, r, r - thickness, breakAngle2, breakAngle3, colour2, backgroundColour);
            tft->drawArc(xPos, yPos, r, r - thickness, breakAngle3, valAngle, colour3, backgroundColour);
        }
    }
    lastAngle = valAngle; // Store meter arc position for next redraw
    if (displayValue)
    {
        tft->setTextSize(2);
        tft->setTextColor(textColour);
        int h = tft->fontHeight();
        tft->setTextDatum(TC_DATUM);
        tft->fillRect(xPos - radius, yPos + radius, radius * 2, h, backgroundColour);
        char buffer[50];
        sprintf(buffer, "%i%s", val, units);
        uint16_t w = tft->drawString(buffer, xPos, yPos + radius);
    }
}