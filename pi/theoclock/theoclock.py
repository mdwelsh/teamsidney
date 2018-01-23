#!/usr/bin/env python

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

def loadImages(imageList):
  images = [Image.open(fn) for fn in imageList]
  pixels = [[(0, 0, 0) for y in range(height)] for x in range(width * len(imageList))]
  for i, img in enumerate(images):
    x_offset = width * i
    for x in range(width):
      for y in range(height):
        pixels[x + x_offset][y] = img.getpixel((x, y))
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

def showImage(imageFile):
  img = Image.open(imageFile)
  for x in range(width):
    for y in range(height):
      pixel = img.getpixel((x, y))
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      unicorn.set_pixel(x, y, r, g, b)
  unicorn.show()

#showImage('bb9e.png')
slideShow(['bb8.png', 'ls1.png', 'bb9e.png', 'ls2.png', 'stormtrooper.png'], 0.1)

while True:
  unicorn.show()
  time.sleep(1)
