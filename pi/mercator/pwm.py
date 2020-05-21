#!/usr/bin/python3

import time
import datetime
import signal

import gpiozero

device = gpiozero.PWMOutputDevice(pin=17)

duty_cycle = 0.0
x = 0
while True:
	device.on()
	duty_cycle = (x % 100) * 0.01

	#if (x % 2 == 0):
	#    duty_cycle = 0.00
	#else:
	#    duty_cycle = 0.75
	print(f"Duty cycle is {duty_cycle}")
	device.value = duty_cycle
	time.sleep(0.1)
	x += 1
