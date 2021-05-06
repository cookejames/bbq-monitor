#include <buttons.h>
#include <EasyButton.h>
#include <config.h>
#include <ArduinoLog.h>

namespace buttons
{
  static EasyButton upButton(BUTTON_UP_PIN, 35, true, false);
  static EasyButton downButton(BUTTON_DOWN_PIN, 35, true, false);
  static Controller *controller;

  void buttonISR()
  {
    downButton.read();
  }

  void upShortPressed()
  {
    Log.trace("Up button short pressed");
    // controller->increaseSetpoint();
  }

  void upLongPressed()
  {
    Log.trace("Up button long pressed");
    // controller->toggleStartupMode();
  }

  void downShortPressed()
  {
    Log.trace("Down button short pressed");
    // controller->decreaseSetpoint();
  }

  void downLongPressed()
  {
    Log.trace("Down button long pressed");
  }

  void setup(Controller *_controller)
  {
    controller = _controller;
    downButton.enableInterrupt(buttonISR);
    upButton.onPressed(upShortPressed);
    upButton.onPressedFor(BUTTON_LONG_PRESS_DURATION, upLongPressed);
    downButton.onPressed(downShortPressed);
    downButton.onPressedFor(BUTTON_LONG_PRESS_DURATION, downLongPressed);
  }

  void check()
  {
    // downButton.update();
    // upButton.read();
    // downButton.read();
  }
}