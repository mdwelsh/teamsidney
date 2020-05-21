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
pwm.value = 0.1
blink_t1 = time.time()

while True:
  avg = rate_avg
  if avg == 0.0:
    avg = 0.001
  period = 1.0 / avg

  now = time.time()
  delta = now - blink_t1
  if delta > period:
    print(f"rate {avg} period {period} delta {delta}")
    blink_t1 = now
    for index in range(NUM_PIXELS):
      strip[index] = 0xFF0000
    strip.show()

    for index in range(NUM_PIXELS):
      strip[index] = 0x000000
    strip.show()
