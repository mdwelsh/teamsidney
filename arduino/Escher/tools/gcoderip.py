#!/usr/bin/python

import fileinput
import math
import re
import sys

import matplotlib.pyplot as plt

# Map from gcode coordinates to step coordinates.
def mmToSteps(pt):
  x, y = pt
  return (int(x * 3.), int(y * 3.))

def atan3(dy, dx):
 a = math.atan2(dy,dx)
 if a < 0:
   a = (math.pi * 2.0) + a
 return a

# Adapted from:
# https://www.marginallyclever.com/2014/03/how-to-improve-the-2-axis-cnc-gcode-interpreter-to-understand-arcs/
def doArc(posx, posy, x, y, cx, cy, cw, CM_PER_SEGMENT=0.1): 
  retval = []
  #print >> sys.stderr, '\nArc from (%f, %f) to (%f, %f) center (%f, %f) cw %s' % (posx, posy, x, y, cx, cy, cw)
  dx = posx - cx 
  dy = posy - cy
  radius = math.sqrt((dx*dx)+(dy*dy))
  #print >> sys.stderr, 'Radius %f' % radius

  # find the sweep of the arc
  angle1 = atan3(posy - cy, posx - cx)
  angle2 = atan3(y - cy, x - cx)
  sweep = angle2 - angle1
  #print >> sys.stderr, 'Angle1 %f Angle2 %f sweep %f Radius %f' % (angle1, angle2, sweep, radius)

  if sweep < 0 and cw:
    angle2 += 2.0 * math.pi
  elif sweep > 0 and not cw:
    angle1 += 2.0 * math.pi

  sweep = angle2 - angle1
  #print >> sys.stderr, 'Now Angle1 %f Angle2 %f sweep %f Radius %f' % (angle1, angle2, sweep, radius)

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
  waypoints = [(0.0, 0.0)]

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
        # Docs say that G02 is clockwise, but maybe my math is wrong (or I'm flipped around
        # in the y-axis) since G03 needs to be CW for this to work.
        cw = True
      curve = doArc(curx, cury, x, y, curx+i, cury+j, cw)
      for pt in curve:
        waypoints.append(pt)

    #print >> sys.stderr, 'Cannot parse: ' + line

  return waypoints

waypoints = parseGcode(fileinput.input())

# Map to steps and filter out duplicates.

waypoints = map(mmToSteps, waypoints)

# Remove duplicates.
prev = None
positions = []
for point in waypoints:
  if prev == None or point != prev:
    prev = point
    positions.append(point)

print '#define GCODE_NUM_POINTS %d' % len(positions)
print 'const std::pair<long, long> _GCODE_POINTS[%d] = {' % len(positions)
for (x, y) in positions:
  print '  { std::make_pair(%d, %d) },' % (x, y)
print '};'

xes = [x for (x, y) in positions]
yes = [y for (x, y) in positions]
plt.plot(xes, yes)
plt.show()
