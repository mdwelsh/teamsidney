#!/usr/bin/python3

import time
import datetime
import signal

import gpiozero

t1 = time.time()
t_avg = t1
alpha = 0.7
rate_avg = 0.0

def detect():
    global t1
    global rate_avg

    dt = time.time()
    rate = 1.0 / (dt-t1)
    rate_avg = alpha * rate_avg + (1.0 - alpha) * rate
    print(f'Detect, rate {rate:.2f} Hz, avg {rate_avg:.2f} Hz\r', end='')
    t1 = dt

sensor = gpiozero.LineSensor(pin=17, pull_up=True, queue_len=1, sample_rate=1000)
sensor.when_line = detect
print('Waiting for input...')
signal.pause()

