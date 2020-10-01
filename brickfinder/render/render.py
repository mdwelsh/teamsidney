#!/usr/bin/env python

import os
import random

import ldraw


PARTS_DIR = "/Users/mdw/Downloads/Bricksmith/ldraw/parts"

parts_files = [f for f in os.listdir(PARTS_DIR) if os.path.isfile(os.path.join(PARTS_DIR, f))]

print("0 untitled model")
print("0 Name:")
print("0 Author: Matt Welsh")

for _ in range(100):
  color = random.randint(1, 4)
  x = random.randint(0, 1000)
  y = random.randint(0, 200)
  z = random.randint(0, 1000)
  part = random.choice(parts_files)
  #part = '3005.dat'
  print(f"1 {color} {x} {y} {z} 1 0 0 0 1 0 0 0 1 {part}")



