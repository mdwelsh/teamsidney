#!/usr/bin/env python

import random

try:
  import unicornhathd as unicorn
  print("unicorn hat hd detected")
except ImportError:
  from unicorn_hat_sim import unicornhathd as unicorn

while True:
  unicorn.set_pixel(
      random.randint(0, 15),
      random.randint(0, 15),
      random.randint(0, 255),
      random.randint(0, 255),
      random.randint(0, 255))
  unicorn.show()
