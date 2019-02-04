#!/usr/bin/python

# This script converts GCode into a header file that
# contains an array of {x, y} coordinates representing
# stepper motor positions. It understands simple GCode
# commands (G01, G02, and G03 only) and ignores all others
# in the file.

import fileinput
import math
import re
import sys

import matplotlib.pyplot as plt

# Width and height of Etch-a-Sketch in step units.
# You can determine this experimentally (and it depends
# on things like gearing, which steppers are being used,
# etc.)
WIDTH_STEPS = 700
HEIGHT_STEPS = 500


def atan3(dy, dx):
 a = math.atan2(dy,dx)
 if a < 0:
   a = (math.pi * 2.0) + a
 return a


# Adapted from:
# https://www.marginallyclever.com/2014/03/how-to-improve-the-2-axis-cnc-gcode-interpreter-to-understand-arcs/
def doArc(posx, posy, x, y, cx, cy, cw, CM_PER_SEGMENT=0.1): 
  retval = []
  dx = posx - cx 
  dy = posy - cy
  radius = math.sqrt((dx*dx)+(dy*dy))

  # find the sweep of the arc
  angle1 = atan3(posy - cy, posx - cx)
  angle2 = atan3(y - cy, x - cx)
  sweep = angle2 - angle1

  if sweep < 0 and cw:
    angle2 += 2.0 * math.pi
  elif sweep > 0 and not cw:
    angle1 += 2.0 * math.pi

  sweep = angle2 - angle1

  # get length of arc
  l = abs(sweep) * radius
  num_segments = int(math.floor( l / CM_PER_SEGMENT ))

  for i in range(num_segments):
    # interpolate around the arc
    fraction = (i * 1.0) / (num_segments * 1.0)
    angle3 = (sweep * fraction) + angle1

    # find the intermediate position
    nx = cx + math.cos(angle3) * radius
    ny = cy + math.sin(angle3) * radius

    # make a line to that intermediate position
    retval.append((nx, ny))

  # one last line hit the end
  retval.append((x, y))
  return retval


def parseGcode(input):
  waypoints = []

  for line in input:
    m = re.match('(G0[01]) X([\d\.]+) Y([\d\.]+)', line)
    if m:
      x = float(m.group(2))
      y = float(m.group(3))
      waypoints.append((x, y))

    m = re.match('(G0[23]) X([-\d\.]+) Y([-\d\.]+) (Z[-\d\.]+)? I([-\d\.]+) J([-\d\.]+)', line)
    if m:
      if len(waypoints) == 0:
        print sys.stderr, 'Warning! Unable to execute G02/G03 without known previous position.'
        print sys.stderr, line
        continue
      (curx, cury) = waypoints[-1]
      x = float(m.group(2))
      y = float(m.group(3))
      i = float(m.group(5))
      j = float(m.group(6))
      cw = False
      if m.group(1) == 'G03':
        # Docs say that G02 is clockwise, but maybe my math is wrong
        # (or I'm flipped around in the y-axis) since G03 needs to
        # be CW for this to work.
        cw = True
      curve = doArc(curx, cury, x, y, curx+i, cury+j, cw)
      for pt in curve:
        waypoints.append(pt)


  # Trim off (0, 0) point tacked on at end by Inkscape GCode plugin
  if waypoints[-1] == (0., 0.):
    waypoints = waypoints[:-1]

  return waypoints


def scaleToScreen(pts):
  # Find min and max ranges.
  minx = min([x for (x, y) in pts])
  maxx = max([x for (x, y) in pts])
  miny = min([y for (x, y) in pts])
  maxy = max([y for (x, y) in pts])
  dx = maxx - minx
  dy = maxy - miny
  # Scale longest axis to fit.
  if dx > dy:
    scale = WIDTH_STEPS / dx
  else:
    scale = HEIGHT_STEPS / dy
  ret = []
  for (x, y) in pts:
    tx = (x-minx) * scale
    ty = (y-miny) * scale
    ret.append([tx, ty])
  return ret


def removeDuplicates(pts):
  prev = None
  ret = []
  for point in pts:
    if prev == None or point != prev:
      prev = point
      ret.append(point)
  return ret


# Parse and process the input file.
waypoints = parseGcode(fileinput.input())
waypoints = scaleToScreen(waypoints)
waypoints = removeDuplicates(waypoints)

# Write out the output.
print '#define GCODE_NUM_POINTS %d' % len(waypoints)
print 'const std::pair<long, long> _GCODE_POINTS[%d] = {' % len(waypoints)
for (x, y) in waypoints:
  print '  { std::make_pair(%d, %d) },' % (x, y)
print '};'

# Draw it on the screen.
xes = [x for (x, y) in waypoints]
yes = [y for (x, y) in waypoints]
plt.plot(xes, yes)
plt.axes().set_aspect('equal')
plt.show()
