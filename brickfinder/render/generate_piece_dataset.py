#!/usr/bin/env python

import os
import random
import tempfile

from ldraw.figure import *
from ldraw.library.colours import *
from ldraw.library.parts.brick import *
from ldraw.pieces import Piece
from ldraw.tools import get_model
from ldraw.writers.povray import POVRayWriter


PARTS = [
    Brick1X1,
    Brick1X2,
    Brick1X3,
    Brick1X6,
    Brick2X2,
    Brick2X3,
    Brick2X4,
]

COLORS = list(ColoursByCode.values())

NUM_PARTS = 1000

POV_TRAILER = """
light_source
{ <0, 1900, 0>, 1
  fade_distance 1600 fade_power 2
  area_light x*70, y*70, 20, 20 circular orient adaptive 0 jitter
}

light_source
{ <2000, 1700, 0>, <1,.8,.4>
  fade_distance 1000 fade_power 2
  area_light x*70, y*70, 20, 20 circular orient adaptive 0 jitter
}

background { color White }

camera {
  location <-500.000000, 700.000000, -500.000000>
  look_at <500.000000, -100.000000, 500.000000>
  angle 50
}
"""


def gen_parts():
  parts = []
  for _ in range(NUM_PARTS):
    part = random.choice(PARTS)
    color = random.choice(COLORS)

    x = random.randint(0, 1000)
    y = random.randint(0, 100)
    z = random.randint(0, 1000)

    xrot = random.randint(0, 360) - 180
    yrot = random.randint(0, 360) - 180
    zrot = random.randint(0, 360) - 180
    rot = Identity().rotate(x, XAxis).rotate(yrot, YAxis).rotate(zrot, ZAxis)

    parts.append(Piece(color, Vector(x, y, z), rot, part))
  return parts


def gen_pov(ldraw_path, pov_path):
  model, parts = get_model(ldraw_path)

  with open(pov_path, "w") as pov_file:
    pov_file.write('#include "colors.inc"\n\n')
    writer = POVRayWriter(parts, pov_file)
    writer.write(model)
    pov_file.write(POV_TRAILER + '\n')


def main():

  import argparse

parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('integers', metavar='N', type=int, nargs='+',
                    help='an integer for the accumulator')
parser.add_argument('--sum', dest='accumulate', action='store_const',
                    const=sum, default=max,
                    help='sum the integers (default: find the max)')

args = parser.parse_args()
print(args.accumulate(args.integers))


  parts = gen_parts()
  with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.ldr') as ldr:
    for part in parts:
      ldr.write(str(part) + '\n')
  with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.pov') as pov:
    pov.close()
    gen_pov(ldr.name, pov.name)
  print(f"povray -i{pov.name} +W1024 +H768 +fp -o- > o.png && open o.png")


if __name__ == "__main__":
    main()
