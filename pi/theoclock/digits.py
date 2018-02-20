#!/usr/bin/python

one = '''
  X
  X
  X
  X
  X
'''

two = '''
  XXXXX
      X
  XXXXX
  X
  XXXXX
'''

three = '''
  XXXXX
      X
  XXXXX
      X
  XXXXX
'''

four = '''
  X   X
  X   X
  XXXXX
      X
      X
'''

five = '''
  XXXXX
  X
  XXXXX
      X
  XXXXX
'''

six = '''
  XXXXX
  X
  XXXXX
  X   X
  XXXXX
'''

seven = '''
  XXXXX
      X
      X
      X
      X
'''

eight = '''
  XXXXX
  X   X
  XXXXX
  X   X
  XXXXX
'''

nine = '''
  XXXXX
  X   X
  XXXXX
      X
      X
'''

zero = '''
  XXXXX
  X   X
  X   X
  X   X
  XXXXX
'''

colon = '''
   
  X
   
  X
   
'''

space = '\n     \n     \n     \n     \n     \n'


def stringToBitmap(s):
  rows = s.split('\n')[1:-1]
  height = len(rows)
  width = 0
  for r in rows:
    width = max(len(r), width)

  bits = [[False for x in range(width)] for y in range(height)]

  for y, r in enumerate(rows):
    for x in range(min(len(r), width)):
      if r[x] != ' ':
        bits[y][x] = True
  return bits


def getDigit(digit):
  digits = {}
  digits['0'] = zero
  digits['1'] = one
  digits['2'] = two
  digits['3'] = three
  digits['4'] = four
  digits['5'] = five
  digits['6'] = six
  digits['7'] = seven
  digits['8'] = eight
  digits['9'] = nine
  return stringToBitmap(digits[str(digit)])


def getBitmap(numberAsString):
  ret = []
  for digit in numberAsString:
    ret.append(getDigit(digit))
  return ret

