#!/usr/bin/python

import string
import sys
import time

def kabloom():
  print  '''

  K   K      A     BBBBBB  L      OOO    OOO   M     M  !!
  K  K      A A    B     B L     O   O  O   O  MM   MM  !!
  K K      A   A   B     B L     O   O  O   O  M M M M  !!
  KK       AAAAA   BBBBBB  L     O   O  O   O  M  M  M  !!
  K K     A     A  B     B L     O   O  O   O  M     M  !!
  K  K    A     A  B     B L     O   O  O   O  M     M
  K   K  A       A BBBBBB  LLLLL  OOO    OOO   M     M  !!

  You died. Try again.
'''
  exit(0)




def readline(prompt):
  return raw_input(prompt + ' - Enter password: ')


def phase1():
  input = readline('Phase 1')
  if input != 'Moo':
    kabloom()


def phase2():
  password = 'Team'
  password += ' '
  password += 'Sidney'
  input = readline('Phase 2')
  if input != password:
    kabloom()


def phase3():
  password = ''
  for i in range(5):
    password += str(i)
  input = readline('Phase 3')
  if input != password:
    kabloom()


def phase4():
  foo = 'happyboogers'
  password = foo[5:11]
  input = readline('Phase 4')
  if input != password:
    kabloom()


def phase5():
  foo = 'namohcan'
  x = len(foo)-1    # 7
  password = ''
  while x >= 0:
    password += foo[x]
    x -= 1
  input = readline('Phase 5')
  if input != password:
    kabloom()


def phase6():
  stuff = ['some', 'words', 'chicken', 'mooshroom', 'pickaxe', 'redstone']
  password = stuff[0] + ' ' + stuff[5] + ' ' + stuff[2]
  input = readline('Phase 6')
  if input != password:
    kabloom()


def phase7():
  dic = {}
  dic['cows'] = 'hay'
  dic['chickens'] = 'corn'
  dic['sharks'] = 'people'
  dic['chewbacca'] = 'porgs'

  password = ''
  password = dic['chewbacca'] + ' ' + dic['chickens']
  input = readline('Phase 7')
  if input != password:
    kabloom()


def moo(num):
  if num == 0:
    return ''
  else:
    return 'moo' + moo(num-1)


def phase8():
  password = moo(4)
  input = readline('Phase 8')
  if input != password:
    kabloom()


def phase9():
  string = 'silly salamanders slosh seaweed'
  count = 0
  for i in range(len(string)):
    if string[i] == 's':
      count += 1
    elif string[i] == 'e':
      count -= 1
  password = str(count)
  input = readline('Phase 9')
  if input != password:
    kabloom()


def phase10():
  password = str(6 * 7)
  input = readline('Phase 10')
  if input != password:
    kabloom()




def bomb():
  phases = [phase1, phase2, phase3, phase4, phase5, phase6, phase7,
            phase8, phase9, phase10]

  print '\nThis program is a BOMB.'
  print 'To defuse the bomb you must type in %d passwords.' % len(phases)
  print 'If you get any password wrong, it will explode.'
  print 'Good luck!\n'

  start_phase = 0
  # Start from the phase number given on the command line.
  # For example, running "bomb.py 3" starts with phase 3.
  if len(sys.argv) == 2:
    start_phase = string.atoi(sys.argv[1]) - 1

  for index in range(start_phase, len(phases)):
    phase = phases[index]
    phase()
    print 'Very good!\n'

  print '*** BOMB DEFUSED ***'
  print 'You may now go back to your regularly scheduled life.'
  print 'But before you do so, please hold on...'
  for i in range(10):
    print '\r%d...' % (10 - i),
    sys.stdout.flush()
    time.sleep(1)
  print ''
  kabloom()


bomb()




