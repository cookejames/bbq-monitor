from machine import Pin, PWM
import config

FREQUENCY = 25000

FAN_PIN = Pin(config.FAN_PIN)
FAN_PWM = PWM(FAN_PIN)

FAN_PWM.freq(FREQUENCY)

# Duty set as a percentage
def set_fan_duty(duty):
    percent = int(duty / 100 * 1023)
    FAN_PWM.duty(percent)


def deint():
    FAN_PWM.deinit()

set_fan_duty(100)
