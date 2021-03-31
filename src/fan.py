from machine import Pin, PWM
import config

FREQUENCY = 25000

FAN_PIN = Pin(config.FAN_PIN)
FAN_PWM = PWM(FAN_PIN)

FAN_PWM.freq(FREQUENCY)

# Sets the initial duty
_duty = 100

# Duty set as a percentage (0-100)
def set_duty(d):
    percent = int(d / 100 * 1023)
    FAN_PWM.duty(percent)
    _duty = percent

def duty():
    return _duty

def deint():
    FAN_PWM.deinit()

set_duty(_duty)
