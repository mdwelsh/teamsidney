#!/usr/bin/python

import fileinput
import re

def parseGcode(line):
  m = re.match('G0[0123] X([\d\.]+) Y([\d\.]+)', line)
  if m:
    return float(m.group(1)), float(m.group(2))
  return None

output = []
for line in fileinput.input():
  ret = parseGcode(line)
  if ret:
    x, y = ret
    output.append((int(x*10), int(y*10)))

print '#define GCODE_NUM_POINTS %d' % len(output)
print 'std::pair<int, int> _GCODE_POINTS[%d] = {' % len(output)
for (x, y) in output:
  print '  { std::make_pair(%d, %d) },' % (x, y)
print '};'
