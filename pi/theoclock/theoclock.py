#!/usr/bin/env python

import copy
import datetime
import random
import signal
import time
from sys import exit

import digits

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
  image = [[(0, 0, 0) for x in range(width)] for y in range(height)]
  for x in range(width):
    for y in range(height):
      pixel = pixels.getpixel((x, y))
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      image[x][y] = (r, g, b)
      unicorn.set_pixel(x, y, r, g, b)
  unicorn.show()
  pixels.close()
  return image

def fadeBetween(img1, img2, delay):
  fadeOut(img1, 0.1)
  fadeIn(img2, 0.1)
  fadeOut(img2, 0.1)
  fadeIn(img1, 0.1)

def cometDotCoords(step):
  if step < 0:
    return cometDotCoords((step + ((width-1)*4) + 1))
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
  return (x, y)

def cometDot(step, color):
  (x, y) = cometDotCoords(step)
  r, g, b = color
  unicorn.set_pixel(x, y, r, g, b)

# If mix == 0, then all color1
# If mix == 1, then all color2.
def interpolate(color1, color2, mix):
  r = (color1[0] * (1.0 - mix)) + (color2[0] * mix)
  g = (color1[1] * (1.0 - mix)) + (color2[1] * mix)
  b = (color1[2] * (1.0 - mix)) + (color2[2] * mix)
  return (r, g, b)

def drawComet(step, color, tail, originalImage):
  for t in range(tail):
    r, g, b = color
    fade = 1.0 - (t*1.0/tail)
    x, y = cometDotCoords(step-t)
    ro, go, bo = originalImage[x][y]
    rt, gt, bt = interpolate((ro, go, bo), (r, g, b), fade)
    cometDot(step-t, (int(rt), int(gt), int(bt)))

  # Put back the tail.
  x, y = cometDotCoords(step-tail)
  r, g, b = originalImage[x][y]
  cometDot(step-tail, (r, g, b))


def doClock(clock, dayImage, nightImage, wakeupTime, bedTime, stepTime=0.04):
  tick = 0
  while True:
    if tick == 0:
      showTime()
      today = clock.today().replace(hour=0, minute=0, second=0, microsecond=0).date()
      wakeup = clock.combine(today, wakeupTime)
      sleepy = clock.combine(today, bedTime)
      now = clock.now()
      # Note: This logic assumes two things:
      #  (1) wakeup < sleepy, AND
      #  (2) midnight comes between sleepy and wakeup.
      if now > wakeup and now < sleepy:
        curImage = showImage(dayImage)
      elif now > sleepy:
        curImage = showImage(nightImage)
      elif now < wakeup:
        curImage = showImage(nightImage)

    drawComet(tick, (255, 0, 0), 20, curImage)
    tick += 1
    if (tick >= (width*4)-3):
      tick = 0

    time.sleep(stepTime)
    unicorn.show()


def clear():
  for x in range(width):
    for y in range(height):
      unicorn.set_pixel(x, y, 0, 0, 0)


def combineBitmaps(bitmaps):
  # Assume all bitmaps are the same height.
  h = len(bitmaps[0])
  w = 0
  for bm in bitmaps:
    # Assume all rows within a bitmap are the same width.
    w += len(bm[0])

  result = [[False for x in range(w)] for y in range(h)]
  tx = 0
  for bm in bitmaps:
    for sy in range(len(bm)):
      for sx in range(len(bm[sy])):
        result[sy][tx+sx] = bm[sy][sx]
    tx += len(bm[0])
  return result


def scroll(bitmap, color, stepTime, yoffset):
  h = len(bitmap)
  w = len(bitmap[0])
  r, g, b = color
  for xoffset in range(w):
    for x in range(width):
      for sy in range(h):
        if x+xoffset < w:
          pixel = bitmap[sy][x+xoffset]
        else:
          pixel = False

        if pixel:
          unicorn.set_pixel(x, sy+yoffset, r, g, b)
        else:
          unicorn.set_pixel(x, sy+yoffset, 0, 0, 0)
    time.sleep(stepTime)
    unicorn.show()


def showTime(dt=datetime.datetime.now()):
  clear()
  hour = dt.hour
  minute = dt.minute
  second = dt.second
  if hour > 12:
    hour -= 12
  print '%02d : %02d : %02d' % (hour, minute, second)
  bitmaps = []
  bitmaps.extend(digits.getBitmap('%02d' % hour))
  bitmaps.append(digits.stringToBitmap(digits.colon))
  bitmaps.extend(digits.getBitmap('%02d' % minute))
  bitmaps.append(digits.stringToBitmap(digits.colon))
  bitmaps.extend(digits.getBitmap('%02d' % second))
  bitmap = combineBitmaps(bitmaps)
  scroll(bitmap, (0, 0, 255), 0.06, 5)


def main():
  doClock(
      datetime.datetime, 'bb82.png', 'stormtrooper3.png',
      datetime.time(18, 00, 00), datetime.time(19, 28, 0))

  while True:
    unicorn.show()
  time.sleep(1)

if __name__ == "__main__":
  main()


