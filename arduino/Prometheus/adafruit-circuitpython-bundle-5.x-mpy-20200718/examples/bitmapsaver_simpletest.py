# The MIT License (MIT)
#
# Copyright (c) 2019 Dave Astels for Adafruit Industries
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""Example of using save_bitmap"""

import board
import busio
import digitalio
from displayio import Bitmap, Palette
import adafruit_sdcard
import storage
from adafruit_bitmapsaver import save_pixels

# pylint:disable=invalid-name

print("Setting up SD card")
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
cs = digitalio.DigitalInOut(board.SD_CS)
sdcard = adafruit_sdcard.SDCard(spi, cs)
vfs = storage.VfsFat(sdcard)
storage.mount(vfs, "/sd")

WHITE = 0xFFFFFF
BLACK = 0x000000
RED = 0xFF0000
ORANGE = 0xFFA500
YELLOW = 0xFFFF00
GREEN = 0x00FF00
BLUE = 0x0000FF
PURPLE = 0x800080
PINK = 0xFFC0CB

colors = (BLACK, RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, WHITE)

print("Building sample bitmap and palette")
bitmap = Bitmap(16, 16, 9)
palette = Palette(len(colors))
for i, c in enumerate(colors):
    palette[i] = c

for x in range(16):
    for y in range(16):
        if x == 0 or y == 0 or x == 15 or y == 15:
            bitmap[x, y] = 1
        elif x == y:
            bitmap[x, y] = 4
        elif x == 15 - y:
            bitmap[x, y] = 5
        else:
            bitmap[x, y] = 0

print("Saving bitmap")
save_pixels("/sd/test.bmp", bitmap, palette)
