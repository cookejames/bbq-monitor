#ifndef config_h
#define config_h
#include <ArduinoLog.h>
#include <PID_v1.h>

// #define BOARD_DEVKIT
#define BOARD_TTGO_T5

#define LOG_LEVEL LOG_LEVEL_TRACE

#define STATUS_PIN 2

#ifdef BOARD_DEVKIT
#define SERVO_PIN 26
#define FAN_PWM_PIN 27
// #define FAN_RPM_PIN 13
#define BUTTON_UP_PIN 12
#define BUTTON_DOWN_PIN 25
#endif

#ifdef BOARD_TTGO_T5
#define LILYGO_T5_V213
#define HAS_DISPLAY
#define SERVO_PIN 13
#define FAN_PWM_PIN 15
// #define FAN_RPM_PIN 2
#define BUTTON_UP_PIN 26
#define BUTTON_DOWN_PIN 27
#endif

#define FAN_MIN_PWM 15
#define FAN_MAX_PWM 50

#define SERVO_OPEN_ANGLE 175  // The angle the servo is open at
#define SERVO_CLOSED_ANGLE 12 // The angle the servo is closed at
#define SERVO_MIN_PULSE_WIDTH 1100
#define SERVO_MAX_PULSE_WIDTH 2200

#define STARTUP_MODE_DURATION 5 * 60 * 1000

#define LID_OPEN_MODE_ENABLED true
#define LID_OPEN_MODE_THRESHOLD 0.85         //activate lid open mode if the temperature drops 15%
#define LID_OPEN_MODE_DURATION 5 * 60 * 1000 // 5 minutes

#define PID_MODE DIRECT

#define DISPLAY_MIN_TIME_BETWEEN_UPDATES 5 * 1000
#define DISPLAY_MIN_TIME_BETWEEN_TEMPERATURE_UPDATES 5 * 60 * 1000
#define DISPLAY_MIN_TIME_BETWEEN_BATTERY_UPDATES 5 * 60 * 1000

#define BUTTON_LONG_PRESS_DURATION 1000

#endif