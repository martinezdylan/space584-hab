import time
import smbus

BUS = smbus.SMBus(1)
address = 0x42

def readGPS():
    c = None
    response = []
    try:
        while True: # Newline, or bad char.
            c = BUS.read_byte(address)
            if c == 255:
                return False
            elif c == 10:
                break
            else:
                response.append(c)
#        parseResponse(response)
        print("in:")
        print([chr( x ) for x in response])
    except IOError:
        time.sleep(0.5)
#        connectBus()
    except Exception, e:
        print e

def writeGPS(cmnd):
  print("out:")
  print (["0x%0.2X, " % x for x in cmnd])
  for ch in cmnd:
    BUS.write_byte(address, ch)


cmnd = [0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 
        0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 
        0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 
        0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC]


writeGPS(cmnd)
readGPS()
