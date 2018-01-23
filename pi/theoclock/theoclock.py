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

#unicorn.rotation(90)
#unicorn.brightness(0.5)
width, height = unicorn.get_shape()

def showImage(imageFile):
  img = Image.open(imageFile)
  for x in range(width):
    for y in range(height):
      pixel = img.getpixel((x, y))
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      unicorn.set_pixel(x, y, r, g, b)
#  for o_x in range(int(img.size[0]/width)):
#    for o_y in range(int(img.size[1]/height)):
#      for x in range(width):
#        for y in range(height):
#          pixel = img.getpixel(((o_x*width)+y,(o_y*height)+x))
#          r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
#          unicorn.set_pixel(x, y, r, g, b)
  unicorn.show()

showImage('bb9e.png')

#for x in range(width):
#  for y in range(height):
#    unicorn.set_pixel(x, y, x * 16, y * 16, 0)
#unicorn.show()

while True:
  unicorn.show()
  time.sleep(1)
