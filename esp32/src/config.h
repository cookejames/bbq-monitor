#ifndef config_h
#define config_h
#include <ArduinoLog.h>

#define LOG_LEVEL LOG_LEVEL_TRACE

#define STATUS_PIN 2

#define SERVO_PIN 26
#define SERVO_OPEN 170 // The angle the servo is open at
#define SERVO_CLOSED 0 // The angle the servo is closed at
#define SERVO_MIN_PULSE_WIDTH 1100
#define SERVO_MAX_PULSE_WIDTH 2200

#define FAN_PWM_PIN 27
#define FAN_RPM_PIN 13

#define STARTUP_MODE_PERCENTAGE 1.5
#define STARTUP_MODE_ENABLED true

#endif