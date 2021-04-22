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
#define FAN_RPM_PIN 13
#endif

#ifdef BOARD_TTGO_T5
#define SERVO_PIN 22
#define FAN_PWM_PIN 21
#define FAN_RPM_PIN 13
#endif

#define SERVO_OPEN 180 // The angle the servo is open at
#define SERVO_CLOSED 3 // The angle the servo is closed at
#define SERVO_MIN_PULSE_WIDTH 1100
#define SERVO_MAX_PULSE_WIDTH 2200

#define STARTUP_MODE_ENABLED false
#define STARTUP_MODE_PERCENTAGE 0.7 // startup mode runs until this percentage of the setpoint

#define LID_OPEN_MODE_ENABLED false
#define LID_OPEN_MODE_THRESHOLD 0.85         //activate lid open mode if the temperature drops 15%
#define LID_OPEN_MODE_DURATION 5 * 60 * 1000 // 5 minutes

#define PID_MODE DIRECT

#endif