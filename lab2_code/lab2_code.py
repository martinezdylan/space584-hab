#/usr/bin/env python3
"""

.ABOUT

KGB (Kewl Girls & Boys) | SPACE 584 | 02/11/2020 | High Altitude Balloon

.VERSION CHANGES

KGB (Kewl Girls & Boys) | SPACE 584 | 08/24/2018 | Created Project
                                                 | 1)
.DESCRIPTION

This will take in data from the KGB instruments and dynamically write to file.

"""

import time
from datetime import datetime
import csv
import time
import board
import busio
import threading
import adafruit_gps

SCL = board.SCL
SDA = board.SDA

i2c = busio.I2C(board.SCL, board.SDA)

gps = adafruit_gps.GPS_GtopI2C(i2c, address=66, debug=False)

gps.send_command(b'PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0')

gps.send_command(b'PMTK220,1000')

last_print = time.monotonic()

## definition: creates main CSV file
def createCSV():
  ## initialize file name
  fileName = 'lab2_output.csv'
  ## open unique file
  with open(fileName, 'w', newline='') as file:
    writer = csv.writer(file)

    ## create header
    writer.writerow([
      "timestamp_EPOCHms",
      "ublox_latitude",
      "ublox_longitude",
    ])

  return fileName

def writeCSV(fileName):
  ## open file
  with open(fileName, 'a', newline='') as file:
    writer = csv.writer(file)

    ## create timestamp
    now = datetime.now()
    timestamp = datetime.timestamp(now)

    ## update gps
    gps.update()

    current = time.monotonic()
    if current - last_print >= 1.0:
        last_print = current
        if not gps.has_fix:
            print('Waiting for fix...')
        else:
          ## write line
          writer.writerow([
            ## timestamp
            timestamp,
            ## coordinates (GPS)
            '{0:.6f}'.format(gps.latitude),
            '{0:.6f}'.format(gps.longitude)
          ])

  ## sleep for 5 seconds
  threading.Timer(5.0, writeCSV, [fileName]).start()

  return None

## initialize creation
fileName = createCSV()

## loop: five second iteration to write csv
writeCSV(fileName)