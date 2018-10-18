#!/usr/bin/python

import math
import numpy as np
import svgwrite
from shapely.geometry import LineString

red = svgwrite.rgb(255, 0, 0)
green = svgwrite.rgb(0, 255, 0)
blue = svgwrite.rgb(0, 0, 255)
black = svgwrite.rgb(0, 0, 0)
gray = svgwrite.rgb(100, 100, 100)
purple = svgwrite.rgb(255, 0, 255)
sw = 0.01

class Turtle:
  def __init__(self, dwg, origin=(0., 0.)):
    self.dwg = dwg
    self.angle = 0.
    self.pos = (0., 0.)
    self.origin = origin

  def text(self, string):
    self.dwg.add(self.dwg.text(string, insert=self.pos, fill='black', font_size=0.05))

  def turnTo(self, angle):
    self.angle = angle

  def turnLeft(self, angle):
    self.angle -= angle

  def turnRight(self, angle):
    self.angle += angle

  def absolutePosition(self, pos=None):
    if pos == None:
      pos = self.pos
    (ox, oy) = self.origin
    (tx, ty) = pos
    return (ox+tx, oy+ty)

  def moveTo(self, pos, stroke=None, stroke_width=0.01):
    p1 = self.pos
    self.pos = pos
    if stroke != None:
      print 'Line from %s to %s' % (p1, self.pos)
      self.dwg.add(self.dwg.line(
        self.absolutePosition(p1),
        self.absolutePosition(),
        stroke=stroke, stroke_width=stroke_width))

  def moveRelative(self, delta, stroke=None, stroke_width=0.01):
    (x, y) = self.pos
    (dx, dy) = delta
    self.moveTo((x+dx, y+dy), stroke, stroke_width)

  def forward(self, dist, stroke=black, stroke_width=0.01):
    dx = dist * math.cos(self.angle)
    dy = dist * math.sin(self.angle)
    self.moveRelative((dx, dy), stroke, stroke_width)



def lineTo(start, end, length):
  (sx, sy) = start
  (ex, ey) = end
  print 'lineTo: %s %s %f' % (start, end, length)

  # Vertical line.
  if (ex == sx):
    print 'Slope: infinite'
    tx = sx
    ty = sy + ((ey-sy) * length)
  else:
    slope = (ey-sy)/(ex-sx)
    print 'Slope: ' + str(slope)
    mult = 1.0
    if (length < 0):
      mult = -1.0
    if ex < sx:
      tx = sx - mult * math.sqrt(length**2 / ((slope**2) + 1.0))
    else:
      tx = sx + mult * math.sqrt(length**2 / ((slope**2) + 1.0))
    ty = sy + ((tx - sx) * slope)
  return (tx, ty)


def lineIntersection(l1, l2):
  (s1, e1) = l1
  (s2, e2) = l2
  ls1 = LineString([s1, e1])
  ls2 = LineString([s2, e2])
  ret = ls1.intersection(ls2)
  print 'lineIntersection(%s, %s) -> %s' % (ls1, ls2, ret)
  return (ret.x, ret.y)

def azimuth(point1, point2):
  """ Return angle between two points."""
  (p1x, p1y) = point1
  (p2x, p2y) = point2
  angle = np.arctan2(p2x - p1x, p2y - p1y)
  return angle


def angleBetween(point1, point2, intersection):
  """ Return angle between two lines at the given intersection point."""
  return azimuth(point2, intersection) - azimuth(point1, intersection)


# Returns: (outerPoints, innerPoints)
def drawStar(turtle, numPoints=10, labelPoints=False, stroke=black, stroke_width=0.01):
  outerPoints = []
  a1 = (math.pi * 2.0) / (numPoints * 1.0)

  outerPoints = []
  # Offset by 1/2 of a step.
  angle = a1 / 2.0
  for index in range(numPoints):
    x = 1.0 * math.cos(angle)
    y = 1.0 * math.sin(angle)
    outerPoints.append((x, y))
    angle += a1

  # Radius of inner circle. Worked out using lots of math :-)
  r2 = math.tan(a1) / (math.tan(a1) + math.tan(a1/2))
  print 'r2 is ' + str(r2)

  angle = 0.0
  innerPoints = []
  for index in range(numPoints):
    x = r2 * math.cos(angle)
    y = r2 * math.sin(angle)
    innerPoints.append((x, y))
    angle += a1

  # Draw the star.
  for index in range(numPoints):
    cp = innerPoints[index]
    np = outerPoints[index]
    turtle.moveTo(cp)
    turtle.moveTo(np, stroke=red, stroke_width=sw)

    if labelPoints:
      dwg.add(dwg.text('inner ' + str(index), insert=cp, fill='blue', font_size=0.1))
      dwg.add(dwg.text('outer ' + str(index), insert=np, fill='black', font_size=0.1))

    cp = innerPoints[index]
    np = outerPoints[(index - 1) % numPoints]
    turtle.moveTo(cp)
    turtle.moveTo(np, stroke=red, stroke_width=sw)

  return (outerPoints, innerPoints)


def distance(start, end):
  (sx, sy) = start
  (ex, ey) = end
  return math.sqrt((ex-sx)**2 + (ey-sy)**2)


def extendStar(turtle, outerPoints, innerPoints, length=2.0, stroke=gray, stroke_width=0.01):
  numPoints = len(outerPoints)
  for index in range(numPoints):

    # Extend from innerPoints outwards to outerPoints.
    cp = innerPoints[index]
    np = outerPoints[index]
    tp = lineTo(cp, np, length)
    turtle.moveTo(cp)
    turtle.moveTo(tp, stroke=stroke, stroke_width=stroke_width)

    cp = innerPoints[index]
    np = outerPoints[(index - 1) % numPoints]
    tp = lineTo(cp, np, length) 
    turtle.moveTo(cp)
    turtle.moveTo(tp, stroke=stroke, stroke_width=stroke_width)


def encroachStar(turtle, outerPoints, innerPoints, length=-0.3, stroke=black, stroke_width=0.01):
  print '----- encroachStar -----'
  numPoints = len(outerPoints)
  for index in range(numPoints):
    # Extend from innerPoints inwards to outerPoints.
    ip1 = innerPoints[index]
    op1 = outerPoints[(index - 1) % numPoints]
    ip2 = innerPoints[(index + 1) % numPoints]
    op2 = outerPoints[(index + 1) % numPoints]
    tp1 = lineTo(ip1, op1, -1.0)  # Assume this is long enough
    tp2 = lineTo(ip2, op2, -1.0)  # Assume this is long enough

    #turtle.moveTo(ip1)
    #turtle.moveTo(tp1, stroke=blue, stroke_width=stroke_width)
    #turtle.moveTo(ip2)
    #turtle.moveTo(tp2, stroke=red, stroke_width=stroke_width)

    mp = lineIntersection((ip1, tp1), (ip2, tp2))
    turtle.moveTo(ip1)
    turtle.moveTo(mp, stroke=blue, stroke_width=stroke_width)
    turtle.moveTo(ip2)
    turtle.moveTo(mp, stroke=blue, stroke_width=stroke_width)


def starPentagon(turtle, outerPoints, innerPoints, length=0.3, stroke=black, stroke_width=0.01):
  numPoints = len(outerPoints)
#  for index in range(numPoints):
  for index in [2]:
    ip = innerPoints[index]
    op1 = outerPoints[index]
    op2 = outerPoints[(index - 1) % numPoints]
    az1 = azimuth(ip, op1)
    az2 = azimuth(ip, op2)
    half = az1 + ((az2-az1)/2)
    turtle.moveTo(op1)
    turtle.text('az1 %0.2f' % np.degrees(az1))
    turtle.moveTo(op2)
    turtle.text('az2 %0.2f' % np.degrees(az2))
    turtle.moveTo(ip)
    turtle.text('half %0.2f' % np.degrees(half))
    turtle.turnTo(half)
    turtle.forward(1.0, stroke=green)


def polygon(turtle, sides, sidelen, stroke=black, stroke_width=0.01):
  angle = (2.0 * math.pi) / sides
  for index in range(sides):
    turtle.forward(sidelen, stroke=stroke, stroke_width=stroke_width)
    turtle.turnRight(angle)


def compass(turtle, numPoints=8, length=0.5):
  a1 = (math.pi * 2.0) / (numPoints * 1.0)
  angle = 0.
  for index in range(numPoints):
    x = length * math.cos(angle)
    y = length * math.sin(angle)
    turtle.moveTo((0, 0))
    turtle.moveTo((x, y), stroke=black)
    turtle.text('(%0.2f, %0.2f)' % (x, y))
    angle += a1

  turtle.angle = 0.
  for index in range(numPoints):
    turtle.moveTo((0, 0))
    turtle.forward(length / 2., stroke=red)
    turtle.text('%0.2f' % np.degrees(turtle.angle))
    turtle.turnRight(a1)


dwg = svgwrite.Drawing('out.svg')
t = Turtle(dwg)

compass(t)

#(outerPoints, innerPoints) = drawStar(t, stroke=black, stroke_width=0.02)
#extendStar(t, outerPoints, innerPoints)
#encroachStar(t, outerPoints, innerPoints)
#starPentagon(t, outerPoints, innerPoints)

dwg.save()
