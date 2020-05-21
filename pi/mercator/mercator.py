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

print("Setting strip to blue...")
for index in range(NUM_PIXELS):
  strip[index] = 0x0000FF
strip.show()
time.sleep(2.0)
print("Starting.")


TARGET_RATE = 15.0
ALPHA = 0.3

detect_t1 = time.time()
rate_avg = 0.0

def detect():
    global detect_t1
    global rate_avg
    dt = time.time()
    rate = 1.0 / (dt - detect_t1)
    rate_avg = ALPHA * rate_avg + (1.0 - ALPHA) * rate
    detect_t1 = dt

sensor = gpiozero.LineSensor(pin=27, pull_up=True, queue_len=1, sample_rate=1000)
sensor.when_line = detect

pwm = gpiozero.PWMOutputDevice(pin=17)
pwm.on()
duty_cycle = 0.0
x = 0
adjust_t1 = time.time()

while True:
  


  for index in range(NUM_PIXELS):
    strip[index] = 0xFF0000
  strip.show()

  for index in range(NUM_PIXELS):
    strip[index] = 0x000000
  strip.show()

  dt = time.time()
  if (dt - adjust_t1) > 2.0:
    if rate_avg < TARGET_RATE and duty_cycle < 1.0:
      duty_cycle += 0.05
    elif rate_avg > TARGET_RATE and duty_cycle > 0.0:		
      duty_cycle -= 0.05
    duty_cycle = max(0.0, min(1.0, duty_cycle))
    pwm.value = duty_cycle
    print(f'Speed: {rate_avg:.2f} Hz, setting duty_cycle to {duty_cycle}')
    adjust_t1 = time.time()

