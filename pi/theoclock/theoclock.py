#!/usr/bin/env python3

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

unicorn.rotation(0)
width, height = unicorn.get_shape()

def setPixel(x, y, r, g, b):
  unicorn.set_pixel((width-1)-x, y, r, g, b)

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
        setPixel(x, y, r, g, b)
    unicorn.show()
    time.sleep(delay)
    x_offset += 1


def drawImage(pixels):
  for x in range(width):
    for y in range(height):
      pixel = pixels[x][y]
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      setPixel(x, y, r, g, b)
#  unicorn.show()

def showImage(imageFile):
  pixels = Image.open(imageFile)
  image = [[(0, 0, 0) for x in range(width)] for y in range(height)]
  for x in range(width):
    for y in range(height):
      pixel = pixels.getpixel((x, y))
      r, g, b = int(pixel[0]),int(pixel[1]),int(pixel[2])
      image[x][y] = (r, g, b)
      setPixel(x, y, r, g, b)
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
  setPixel(x, y, r, g, b)

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

# Each span is a tuple (startTime, image, brightness).
# Example:
#   (07:00, 'morning.png', 0.3)
#   (10:00, 'midday.png', 0.5)
#   (17:00, 'afternoon.png', 0.3)
#   (19:00, 'night.png', 0.1)
#
# Logic:
#   Take current time
#   If showClockTime has elapsed, show the clock.
#   Find last entry in list that is BEFORE the current time.
#     (If first entry is after current time, use last list entry.)
#   Set image/brightness to the corresponding entry.

def doClock(clock, spans, stepTime=0.04, showClockTime=datetime.timedelta(seconds=60)):
  tick = 0
  prev = datetime.datetime.fromtimestamp(0)
  while True:
    if tick == 0:
      today = clock.today().replace(hour=0, minute=0, second=0, microsecond=0).date()
      now = clock.now()

      # Every minute, show the time
      if now - prev >= showClockTime:
        showTime(now)
        prev = now

      curSpan = None
      for (startTime, image, brightness) in spans:
        st = clock.combine(today, startTime)
        if st < now:
          curSpan = (startTime, image, brightness)
      if curSpan is None:
        curSpan = spans[-1]

      (_, image, brightness) = curSpan
      unicorn.brightness(brightness)
      curImage = showImage(image)

    drawComet(tick, (255, 0, 0), 20, curImage)
    tick += 1
    if (tick >= (width*4)-3):  # That is, has gone all the way around.
      tick = 0

    time.sleep(stepTime)
    unicorn.show()


def clear():
  for x in range(width):
    for y in range(height):
      setPixel(x, y, 0, 0, 0)


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
          setPixel(x, sy+yoffset, r, g, b)
        else:
          setPixel(x, sy+yoffset, 0, 0, 0)
    time.sleep(stepTime)
    unicorn.show()


def showTime(dt=datetime.datetime.now()):
  clear()
  hour = dt.hour
  minute = dt.minute
  second = dt.second
  if hour > 12:
    hour -= 12
  bitmaps = []
  bitmaps.append(digits.stringToBitmap(digits.space))
  bitmaps.append(digits.stringToBitmap(digits.space))
  bitmaps.extend(digits.getBitmap('%02d' % hour))
  bitmaps.append(digits.stringToBitmap(digits.colon))
  bitmaps.extend(digits.getBitmap('%02d' % minute))
  bitmaps.append(digits.stringToBitmap(digits.colon))
  bitmaps.extend(digits.getBitmap('%02d' % second))
  bitmap = combineBitmaps(bitmaps)
  scroll(bitmap, (0, 255, 0), 0.06, 5)

# Each span is a tuple (startTime, image, brightness).
# Example:
#   (07:00, 'morning.png', 0.3)
#   (10:00, 'midday.png', 0.5)
#   (17:00, 'afternoon.png', 0.3)
#   (19:00, 'night.png', 0.1)
def showSpans(spans):
  for (startTime, image, brightness) in spans:
    unicorn.brightness(brightness)
    showTime(startTime)
    curImage = showImage(image)
    unicorn.show()
    time.sleep(3)

def main():
  spans = [
      (datetime.time(7, 00, 00), 'bb82.png', 0.2),
      (datetime.time(19, 30, 00), 'stormtrooper3.png', 0.05),
  ]
  showSpans(spans)
  doClock(datetime.datetime, spans)

  while True:
    unicorn.show()
  time.sleep(1)

if __name__ == "__main__":
  main()


