#!/usr/bin/python

import math
import svgwrite

red = svgwrite.rgb(255, 0, 0)
green = svgwrite.rgb(0, 255, 0)
blue = svgwrite.rgb(0, 0, 255)
black = svgwrite.rgb(255, 255, 255)
sw = 0.01


def drawStar(dwg, numPoints=10, labelPoints=False):
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
    dwg.add(dwg.line(cp, np, stroke=red, stroke_width=sw))

    if labelPoints:
      dwg.add(dwg.text('inner ' + str(index), insert=cp, fill='blue', font_size=0.1))
      dwg.add(dwg.text('outer ' + str(index), insert=np, fill='black', font_size=0.1))

    cp = innerPoints[index]
    np = outerPoints[(index - 1) % numPoints]
    dwg.add(dwg.line(cp, np, stroke=red, stroke_width=sw))

dwg = svgwrite.Drawing('out.svg')
drawStar(dwg)
dwg.save()
