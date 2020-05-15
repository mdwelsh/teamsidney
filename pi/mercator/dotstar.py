#!/usr/bin/env python3

# WHAT WORKS:
# -----------
# MOSI pin on Pi --> "C" line on connector to DotStar
# CLOCK_PIN = board.MOSI
# SCLK pin on Pi --> "D" line on connector to DotStar
# DATA_PIN = board.SCLK
#
# And I do see stuff on the oscope as expected.
#
# However ... with this setup, the DotStar library falls back to bit-banging (which explains
# why it's so slow). 
#
# So why is SPI not working? Need to investigate.
# Fallback exception is:
#
# MDW: Falling back to non-SPI due to No Hardware SPI on (SCLK, MOSI, MISO)=(10, 11, None)
# Valid SPI ports:((0, 11, 10, 9), (1, 21, 20, 19), (2, 42, 41, 40))
#
# Maybe we should try CLOCK=21, DATA=20?
# SUCCESS!
# For this to work, I had to do two things:
# 1) Edit /boot/config.txt and add the line
#      dtoverlay=spi1-3cs
#    to enable the SPI1 interface.
# 2) Edit /boot/cmdline.txt and remove the entry:
#      console=serial0,115200
#    which apparently interferes with SPI1.

import time

import adafruit_dotstar
import board

NUM_PIXELS=70

# This works:
#CLOCK_PIN=board.MOSI
#DATA_PIN=board.SCLK

# Try this:
#CLOCK_PIN=board.SCLK
#DATA_PIN=board.MOSI
# The above does not work even with the pins swapped to the DotStar connector.
# Check with the oscope to see if the pins are being toggled correctly.
# Nope! I don't see anything on either pin when using this configuration.

# Try this:
CLOCK_PIN=board.SCLK_1
DATA_PIN=board.MOSI_1

strip = adafruit_dotstar.DotStar(CLOCK_PIN, DATA_PIN, NUM_PIXELS, brightness=1.0, auto_write=False)

time.sleep(1)

for index in range(NUM_PIXELS):
  strip[index] = 0x0000FF
strip.show()
time.sleep(0.1)

while True:
  for index in range(NUM_PIXELS):
    strip[index] = 0xFF0000
  strip.show()

  for index in range(NUM_PIXELS):
    strip[index] = 0x000000
  strip.show()
