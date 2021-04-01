from lib.simple_pid import PID
import config


class PidParameters:
    def __init__(self, p=4, i=0.0035, d=5):
        self.p = p
        self.i = i
        self.d = d


class Temperature:
    def __init__(
        self,
        fan,
        servo,
        measuring_sensor="temperature1",
        temperature=0, 
        setpoint=-1, #PID update disabled by default
        pid_parameters=PidParameters(),
        output_limits=(0, 100),
        sample_time=1,
    ):
        self.fan = fan
        self.servo = servo
        self.measuring_sensor = measuring_sensor
        self.temperature = temperature
        self.setpoint = setpoint
        self.pid_parameters = pid_parameters
        self.pid = PID(
            pid_parameters.p,
            pid_parameters.i,
            pid_parameters.d,
            setpoint=self.setpoint,
            output_limits=output_limits,
            sample_time=sample_time,
        )

    # Returns the fan speed and servo opening angle
    def get_fan_and_servo_values(self, value):
        if value > 10:
            return value, config.SERVO_OPEN
        if value == 0:
            return 0, config.SERVO_CLOSED
        return 0, config.SERVO_OPEN

    def update(self):
        if self.setpoint == -1:
            return

        self.pid.tunings = (
            self.pid_parameters.p,
            self.pid_parameters.i,
            self.pid_parameters.d,
        )
        self.pid.setpoint = self.setpoint
        value = int(self.pid(self.temperature))
        fan_speed, servo_angle = self.get_fan_and_servo_values(value)
        self.fan.set_duty(fan_speed)
        self.servo.set_angle(servo_angle)
        print(
            "Temperature: {}C, setpoint: {}C, PID: {}. Setting fan {}%, servo {} deg.".format(
                self.temperature, self.setpoint, value, fan_speed, servo_angle
            )
        )
