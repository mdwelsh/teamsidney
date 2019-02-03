#!/usr/bin/python

import serial
import time

from graphics import *

WIDTH = 700
HEIGHT = 500

curx = 0
cury = HEIGHT

win = GraphWin("serialplot", WIDTH, HEIGHT)

def update_plot(xes, yes):
  pass

def processLine(line):
  global curx, cury
  global win

  if line.startswith('MDW '):
    try:
      vals = line.split()
      x = int(vals[1])
      y = int(vals[2])
      if (x > WIDTH):
        x = WIDTH
      if (y > HEIGHT):
        y = HEIGHT

      y = HEIGHT - y  # Flip upside down
      ln = Line(Point(curx, cury), Point(x, y))
      ln.draw(win)
      curx = x
      cury = y
    except Exception as e:
      print 'Problem plotting line: ' + line,
      print e

ser = None
while ser == None:
  try:
    ser = serial.Serial('/dev/cu.SLAB_USBtoUART', 2000000, timeout=10)
  except Exception as e:
    print 'Waiting for serial port...'
    print e
    time.sleep(1)
    ser = None

curline = ''
while True:
  c = ser.read()
  curline += c
  if c == '\n':
    line = curline
    curline = ''
    processLine(line)

