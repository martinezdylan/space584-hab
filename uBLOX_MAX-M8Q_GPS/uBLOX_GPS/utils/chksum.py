#!/usr/bin/python
import sys
import time

buffer = []
for arg in sys.argv[1:]:
  v = arg
  if v[-1] == ',':
    v = v[:-1]
  buffer.append(int(v,16))

if buffer[0] == 0xB5 and buffer[1] == 0x62:
  buffer.pop(0)
  buffer.pop(0)

CK_A = 0
CK_B = 0
for i in buffer:
  CK_A = CK_A + i
  CK_B = CK_B + CK_A
CK_A &= 0xFF
CK_B &= 0xFF
print(["%02x" % x for x in buffer])
print("chksum 0x%02x, 0x%02x" % (CK_A, CK_B))

