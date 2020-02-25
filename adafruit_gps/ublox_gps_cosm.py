import time
from datetime import datetime
import board
import busio
import csv

import adafruit_gps

SCL = board.SCL
SDA = board.SDA

i2c = busio.I2C(board.SCL, board.SDA)

gps = adafruit_gps.GPS_GtopI2C(i2c, address=66, debug=False)

gps.send_command(b'PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0')

gps.send_command(b'PMTK220,1000')

## create timestamp
now = datetime.now()
timestamp = datetime.timestamp(now)

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

last_print = time.monotonic()
while True:

    gps.update()

    current = time.monotonic()
    if current - last_print >= 1.0:
        last_print = current
        if not gps.has_fix:
            print('Waiting for fix...')
            continue
        print('=' * 40)  # Print a separator line.
        print('Latitude: {0:.6f} degrees'.format(gps.latitude))
        print('Longitude: {0:.6f} degrees'.format(gps.longitude))

        with open(fileName, 'a', newline='') as file:
            writer = csv.writer(file)

            writer.writerow([
            ## timestamp
            timestamp,
            ## coordinates (GPS)
            '{0:.6f}'.format(gps.latitude),
            '{0:.6f}'.format(gps.longitude)
            ])
