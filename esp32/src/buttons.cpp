#include <buttons.h>
#include <config.h>
#include <ArduinoLog.h>

namespace buttons
{
  // Create a class to manage button debounce, short and long presses
  Button::Button(int pin)
  {
    _pin = pin;
    pinMode(_pin, mode == Button::ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  }

  void Button::enableInterrupt(callback_t callback)
  {
    attachInterrupt(digitalPinToInterrupt(_pin), callback, CHANGE);
  }

  void Button::onShortPress(callback_t callback)
  {
    shortPressCb = callback;
  }

  void Button::onLongPress(callback_t callback, uint16_t duration)
  {
    longPressCb = callback;
    longPressDuration = duration;
  }

  void Button::read()
  {
    bool state = digitalRead(_pin);
    bool pressed = state == mode;

    if (millis() < debounceUntil)
    {
      return;
    }
    if (pressed)
    {
      pressedTime = millis();
    }
    if (!pressed)
    {
      uint32_t duration = millis() - pressedTime;
      debounceUntil = millis() + debounceTime;
      if (duration < longPressDuration)
      {
        hasShortPress = true;
      }
      else
      {
        hasLongPress = true;
      }
    }
  }

  void Button::check()
  {
    if (hasShortPress)
    {
      shortPressCb();
      hasShortPress = false;
    }
    if (hasLongPress)
    {
      longPressCb();
      hasLongPress = false;
    }
  }

  //Setup button logic
  Button downButton(BUTTON_DOWN_PIN);
  Button upButton(BUTTON_UP_PIN);
  static Controller *controller;

  void ICACHE_RAM_ATTR downButtonISR()
  {
    downButton.read();
  }

  void ICACHE_RAM_ATTR upButtonISR()
  {
    upButton.read();
  }

  void upShortPressed()
  {
    Log.trace("Up button short pressed");
    // Disable lid open mode on press
    if (controller->isLidOpenMode())
    {
      controller->disableLidOpenMode();
      return;
    }

    if (controller->isAutomaticControl())
    {
      controller->increaseSetpoint();
    }
  }

  void upLongPressed()
  {
    Log.trace("Up button long pressed");
    controller->toggleStartupMode();
  }

  void downShortPressed()
  {
    Log.trace("Down button short pressed");
    // Disable lid open mode on press
    if (controller->isLidOpenMode())
    {
      controller->disableLidOpenMode();
      return;
    }

    if (controller->isAutomaticControl())
    {
      controller->decreaseSetpoint();
    }
  }

  void downLongPressed()
  {
    Log.trace("Down button long pressed");
  }

  void setup(Controller *_controller)
  {
    controller = _controller;
    downButton.enableInterrupt(downButtonISR);
    downButton.onShortPress(downShortPressed);
    downButton.onLongPress(downLongPressed, 500);
    upButton.enableInterrupt(upButtonISR);
    upButton.onShortPress(upShortPressed);
    upButton.onLongPress(upLongPressed, 500);
  }

  void check()
  {
    upButton.check();
    downButton.check();
  }
}