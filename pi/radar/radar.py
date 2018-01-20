#!/usr/bin/env python

import math
import random
import time

try:
  import unicornhathd as unicorn
  print("unicorn hat hd detected")
except ImportError:
  from unicorn_hat_sim import unicornhathd as unicorn

from bresenham import bresenham

center_x = 8
center_y = 8
angle = 0.0

blips = {}

def drawLine(angle, r, g, b):
  end_x = (math.cos(angle) * 20.0) + center_x
  end_y = (math.sin(angle) * 20.0) + center_y
  for x, y in bresenham(center_x, center_y, int(end_x), int(end_y)):
    if x > 0 and x <= 15 and y > 0 and y <= 15:
      unicorn.set_pixel(x, y, r, g, b)

def makeBlip(angle):
  if random.random() < 0.97:
    return
  radius = random.randint(1, 8)
  x = int((math.cos(angle) * radius) + center_x)
  y = int((math.sin(angle) * radius) + center_y)
  blips[angle] = (x, y)
#  unicorn.set_pixel(x, y, 255, 0, 0)

while True:
  angle += 0.1
#  if angle > 2 * math.pi:
#    angle = 0.0

  if angle in blips:
    x, y = blips[angle]
    print 'Deleting blip: %d %d %f' % (x, y, angle)
    unicorn.set_pixel(x, y, 0, 0, 0)
    del blips[angle]

  drawLine(angle-0.3, 0, 50, 0)
  drawLine(angle-0.2, 0, 100, 0)
  drawLine(angle-0.1, 0, 150, 0)
  drawLine(angle, 0, 255, 0)
  makeBlip(angle)
  for blipangle, (x, y) in blips.items():
    dim = int((angle - blipangle) * 30)
    if dim < 0:
      dim = 0
    if dim > 255:
      dim = 255

    unicorn.set_pixel(x, y, 255 - dim, 0, 0)

  unicorn.show()
  time.sleep(0.02)
  drawLine(angle-0.3, 0, 0, 0)
  drawLine(angle-0.2, 0, 0, 0)
  drawLine(angle-0.1, 0, 0, 0)
  drawLine(angle, 0, 0, 0)

