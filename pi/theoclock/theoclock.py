#!/usr/bin/env python

import copy
import datetime
import random
import signal
import time
from sys import exit

try:
  import unicornhathd as unicorn
  print("unicorn hat hd detected")
except ImportError:
  from unicorn_hat_sim import unicornhathd as unicorn

try:
    from PIL import Image
except ImportError:
    exit("This script requires the pillow module\nInstall with: sudo pip install pillow")

unicorn.brightness(0.5)
width, height = unicorn.get_shape()

def loadImage(imageFile):
  return loadImages([imageFile])

def applyFade(pixels, mult):
  newpixels = copy.deepcopy(pixels)
  for x in range(width):
    for y in range(height):
      pixel = newpixels[x][y]
      newpixel = (pixel[0] * mult, pixel[1] * mult, pixel[2] * mult)
      newpixels[x][y] = newpixel
  return newpixels

def fadeOut(img, delay):
  pixels = loadImage(img)
  mult = 1.0
  while mult > 0.0:
    fp = applyFade(pixels, mult)
    drawImage(fp)
    mult = mult - 0.1
    time.sleep(delay)

def fadeIn(img, delay):
  pixels = loadImage(img)
  mult = 0.0
  while mult <= 1.0:
    fp = applyFade(pixels, mult)
    drawImage(fp)
    mult = mult + 0.1
    time.sleep(delay)

def loadImages(imageList):
  images = [Image.open(fn) for fn in imageList]
  pixels = [[(0, 0, 0) for y in range(height)] for x in range(width * len(imageList))]
  for i, img in enumerate(images):
    x_offset = width * i
    for x in range(width):
      for y in range(height):
        pixels[x + x_offset][y] = img.getpixel((x, y))
    img.close()
  return pixels

def slideShow(imageList, delay):
  max_x = width * len(imageList)
  pixels = loadImages(imageList)
  x_offset = 0
  while True:
    if (x_offset % width) == 1:
      unicorn.show()
      time.sleep(5 * delay)
    for x in range(width):
      for y in range(height):
        px = (x + x_offset) % max_x
        py = y
        (r, g, b, a) = pixels[px][py]
        unicorn.set_pixel(x, y, r, g, b)
    unicorn.show()
    time.sleep(delay)
    x_offset += 1

def drawImage(pixels):
  for x in range(width):
    for y in range(height):
      pixel = pixels[x][y]
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      unicorn.set_pixel(x, y, r, g, b)
#  unicorn.show()

def showImage(imageFile):
  pixels = Image.open(imageFile)
  for x in range(width):
    for y in range(height):
      pixel = pixels.getpixel((x, y))
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      unicorn.set_pixel(x, y, r, g, b)
  unicorn.show()
  pixels.close()

def fadeBetween(img1, img2, delay):
  fadeOut(img1, 0.1)
  fadeIn(img2, 0.1)
  fadeOut(img2, 0.1)
  fadeIn(img1, 0.1)

#    +-----+
#    |01234|   if step < width: x = step, y = 0
#    |    5|   elif step < (width*2)-1: x = width-1, y = step-(width-1)
#    |    6|   elif step < (width*3)-2: x = (width*3-2)-(step+1), y = width-1
#    |111 7|   else: x = 0, y = 12 - (step-(width * 3)-2)
#    |21098|
#    +-----+

def drawComet(step, color, tail):
  for t in range(tail):
    r, g, b = color
    fade = 1.0 - (t*1.0/tail)
    cometDot(step-t, (int(r * fade), int(g * fade), int(b * fade)))

def cometDot(step, color):
  if step < 0:
    return
  if step < width:
    x = step
    y = 0
  elif step < (width*2)-1:
    x = width-1
    y = step-(width-1)
  elif step < (width*3)-2:
    x = ((width*3)-2)-(step+1)
    y = width-1
  else:
    x = 0
    y = ((width*3)-2) - (step+1)
  r, g, b = color
  unicorn.set_pixel(x, y, r, g, b)
#  unicorn.show()

def doClock(clock, dayImage, nightImage, bedTime, wakeupTime):
  tick = 0
  while True:
    if tick == 0:
      today = clock.today().replace(hour=0, minute=0, second=0, microsecond=0).date()
      wakeup = clock.combine(today, wakeupTime)
      sleepy = clock.combine(today, bedTime)
      now = clock.now()
      print 'It is now ' + str(now)
      print 'wakeup is ' + str(wakeup)
      print 'sleepy is ' + str(sleepy)
      print ''

      # Note: This logic assumes two things:
      #  (1) wakeup < sleepy, AND
      #  (2) midnight comes between sleepy and wakeup.
      if now > wakeup and now < sleepy:
        showImage(dayImage)
      elif now > sleepy:
        showImage(nightImage)
      elif now < wakeup:
        showImage(nightImage)

    drawComet(tick, (255, 0, 0), 20)
    tick += 1
    if (tick >= (width*4)-3):
      tick = 0

    time.sleep(0.02)
    unicorn.show()

def main():
  doClock(
      datetime.datetime, 'stormtrooper2.png', 'stormtrooper2.png',
      datetime.time(21, 00, 00), datetime.time(20, 1, 0))

  while True:
    unicorn.show()
  time.sleep(1)

if __name__ == "__main__":
  main()


