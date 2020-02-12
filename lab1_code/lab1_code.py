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
import csv
import board
import busio
import threading
import adafruit_am2320
import adafruit_bno055
import adafruit_bmp3xx

## initialize I2C
i2c = busio.I2C(board.SCL, board.SDA)

## initialize components
am2320 = adafruit_am2320.AM2320(i2c)
bno055 = adafruit_bno055.BNO055(i2c)
bmp388 = adafruit_bmp3xx.BMP3XX_I2C(i2c)

## definition: creates main CSV file
def createCSV():
  ## intitialize file name
  fileName = 'lab1_output.csv'
  ## open unique file
  with open(fileName, 'w', newline='') as file:
    writer = csv.writer(file)

    ## create header
    writer.writerow([
      "am2320_humidity",
      "am2320_temperature",
      "bmp388_temperature",
      "bmp388_pressure",
      "bno055_accelerometer",
      "bno055_eulerAngle",
      "bno055_gravity",
      "bno055_gyroscope",
      "bno055_linearAcceleration",
      "bno055_magnetometer",
      "bno055_temperature",
      "bno055_quaternion"
    ])

  return fileName

def writeCSV(fileName):
  ## open file
  with open(fileName, 'a', newline='') as file:
    writer = csv.writer(file)

    ## write line
    writer.writerow([
      ## am2320 (Humidity Sensor)
      am2320.relative_humidity,
      am2320.temperature,
      ## bmp388 (Barometric Pressure Sensor)
      bmp388.temperature,
      bmp388.pressure,
      ## bno055 (IMU)
      bno055.accelerometer,
      bno055.eulerAngle,
      bno055.gravity,
      bno055.gyroscope,
      bno055.linearAcceleration,
      bno055.magnetometer,
      bno055.temperature,
      bno055.quaternion
    ])

  ## sleep for 5 seconds
  threading.Timer(5.0, writeCSV, [fileName]).start()

  return None

## intitialize creation
fileName = createCSV()

## loop: five second iteration to write csv
writeCSV(fileName)