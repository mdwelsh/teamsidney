#!/usr/bin/python3

import time
import datetime
import signal

import adafruit_dotstar
import board
import gpiozero

NUM_PIXELS=70
CLOCK_PIN=board.SCLK_1
DATA_PIN=board.MOSI_1
strip = adafruit_dotstar.DotStar(CLOCK_PIN, DATA_PIN, NUM_PIXELS, brightness=1.0, auto_write=False)

print("Setting strip to black")
for index in range(NUM_PIXELS):
  strip[index] = 0x000000
strip.show()

print("Setting PWM to 0.0")
pwm = gpiozero.PWMOutputDevice(pin=17)
pwm.on()
pwm.value = 0.0
