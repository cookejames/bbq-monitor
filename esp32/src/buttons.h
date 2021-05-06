#ifndef buttons_h
#define buttons_h
#include <Arduino.h>
#include <controller.h>

namespace buttons
{
  class Button
  {
  public:
    Button(int);
    typedef void (*callback_t)();
    void enableInterrupt(callback_t);
    void onShortPress(callback_t);
    void onLongPress(callback_t, uint16_t);
    void read();
    void check();

  private:
    static const bool ACTIVE_HIGH = true;
    static const bool ACTIVE_LOW = false;
    int _pin;
    bool mode = Button::ACTIVE_LOW;
    uint16_t debounceTime = 35;
    uint32_t pressedTime = 0;
    uint32_t debounceUntil = 0;
    uint16_t longPressDuration = 1000;
    callback_t shortPressCb = []() {};
    callback_t longPressCb = []() {};
    bool hasShortPress = false;
    bool hasLongPress = false;
  };

  void setup(Controller *);
  void check();
}

#endif