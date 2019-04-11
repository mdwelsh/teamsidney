#!/usr/bin/env python

import math
import random
import time

try:
  import unicornhathd as unicorn
  print("unicorn hat hd detected")
except ImportError:
  from unicorn_hat_sim import unicornhathd as unicorn

width = 16
height = 16
x = 0
y = 0

def randcolor():
  b = random.randint(0, 255)
  return b

b = randcolor()
while y <= 15:
  while x <= 15:
    b = randcolor()
    unicorn.set_pixel(x, y, 0, 0, b)
    unicorn.show()
    x = x+1
  x = 0
  y = y+1

while True:
  unicorn.show()

#     0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
# 0   X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X 
# 1 
# 2
# 3
# 4
# 5
# 6
# 7
# 8
# 9
# 10
# 11
# 12
# 13
# 14
# 15



