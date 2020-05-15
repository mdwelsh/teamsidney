#!/usr/bin/python3

import time
import datetime
import signal

import gpiozero

device = gpiozero.PWMOutputDevice(pin=17)

for x in range(100):
	device.on()
	if (x % 2 == 0):
	    duty_cycle = 0.75
	else:
	    duty_cycle = 0.25

	print(f"Duty cycle is {duty_cycle}")
	device.value = duty_cycle
	time.sleep(1)
