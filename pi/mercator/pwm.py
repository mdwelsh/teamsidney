#!/usr/bin/python3

import time
import datetime
import signal

import gpiozero

device = gpiozero.PWMOutputDevice(pin=17)

duty_cycle = 0.7
print("Setting duty cycle to {duty_cycle}")
device.on()
device.value = duty_cycle
time.sleep(10.0)
print("Turning off")
device.value = 0.0
device.off()
