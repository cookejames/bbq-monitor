from machine import Pin, PWM
import config


class Servo:
    MIN_US = 1000
    MAX_US = 2000
    MIN_ANGLE = 0
    MAX_ANGLE = 180
    DUTY_FUNC_MIN_VALUE = 0
    DUTY_FUNC_MAX_VALUE = 1023

    def __init__(self, angle=0, pin=config.SERVO_PIN, frequency=50):
        self.frequency = frequency
        self.pin = Pin(pin)
        self.pwm = PWM(self.pin)
        self.pwm.freq(self.frequency)
        self.set_angle(angle)

    def set_angle(self, angle):
        if isinstance(angle, int) is False:
            raise Exception('angle must be an integer, received', angle)

        # For a 50Hz servo with a minimum period of 1ms and max period of 2ms
        # the period would be 1/50 = 20ms. The pulse minimums and maximums
        # would be 1/20 and 2/20, 0.05 and 0.20 eg 5% and 10%.
        # The duty function accepts inputs of 0-1023 so the acceptable range
        # is 5% to 10% of 1023. This can then be mapped to the range of angles the servo
        # is able to move.
        servo_period_us = 1 / self.frequency * 1000 * 1000

        min_duty_as_input = (Servo.MIN_US / servo_period_us) * (
            Servo.DUTY_FUNC_MAX_VALUE - Servo.DUTY_FUNC_MIN_VALUE
        )
        max_duty_as_input = (Servo.MAX_US / servo_period_us) * (
            Servo.DUTY_FUNC_MAX_VALUE - Servo.DUTY_FUNC_MIN_VALUE
        )
        input_range = max_duty_as_input - min_duty_as_input
        input_per_deg = input_range / (Servo.MAX_ANGLE - Servo.MIN_ANGLE)

        if angle > Servo.MAX_ANGLE:
            angle = Servo.MAX_ANGLE
        if angle < Servo.MIN_ANGLE:
            angle = Servo.MIN_ANGLE

        duty = int(angle * input_per_deg + min_duty_as_input)
        self.pwm.duty(duty)

    def stop(self):
        self.pwm.deinit()
