from machine import Pin, PWM
import config


class Fan:
    def __init__(self, duty=100, pin=config.FAN_PIN, frequency=25000):
        self.frequency = frequency
        self.pin = Pin(pin)
        self.pwm = PWM(self.pin)
        self.pwm.freq(self.frequency)
        self.set_duty(duty)

    def set_duty(self, duty):
        if isinstance(duty, int) is False:
            raise Exception('duty must be an integer, received', duty)
        self._duty = duty
        self.pwm.duty(int(duty / 100 * 1023))

    def duty(self):
        return self._duty

    def stop(self):
        self.pwm.deinit()
