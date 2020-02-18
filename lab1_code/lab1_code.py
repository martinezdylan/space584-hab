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
import math
import board
import busio
import threading
import digitalio
import RPi.GPIO as GPIO
import adafruit_am2320
import adafruit_bno055
import adafruit_bmp3xx
import adafruit_mcp3xxx.mcp3008 as MCP
from adafruit_mcp3xxx.analog_in import AnalogIn

## initialize I2C bus
i2c = busio.I2C(board.SCL, board.SDA)

## initialize SPI bus && chip select
spi = busio.SPI(clock=board.SCK, MISO=board.MISO, MOSI=board.MOSI)
cs = digitalio.DigitalInOut(board.D22)

## initialize components
am2320 = adafruit_am2320.AM2320(i2c)
bno055 = adafruit_bno055.BNO055(i2c)
bmp388 = adafruit_bmp3xx.BMP3XX_I2C(i2c)
mcp3008 = MCP.MCP3008(spi, cs)

## analog input channel (thermistor)
chan0 = AnalogIn(mcp3008, MCP.P0)

## conversion coefficients (thermistor)
therm_a = 0.001468
therm_b = 0.0002383
therm_c = 0.0000001007

## voltage & resistor (thermistor)
therm_v_in = 3.3
therm_r2 = 10e3

## definition: creates main CSV file
def createCSV():
  ## intitialize file name
  fileName = 'lab1_output.csv'
  ## open unique file
  with open(fileName, 'w', newline='') as file:
    writer = csv.writer(file)

    ## create header
    writer.writerow([
      "timestamp_EPOCHms",
      "am2320_humidity_%",
      "am2320_temperature_C",
      "bmp388_temperature_C",
      "bmp388_pressure_hPa",
      "bmp388_altitude_m",
      "bno055_acceleration_m/s^2",
      "bno055_euler_angle",
      "bno055_gravity_m/s^2",
      "bno055_gyroscope",
      "bno055_linear_acceleration_m/s^2",
      "bno055_magnetometer_mT",
      "bno055_temperature_C",
      "bno055_quaternion",
      "mcp3008_raw_adc",
      "mcp3008_adc_voltage_V",
      "mcp3008_thermistor_resistance_Ω",
      "mcp3008_thermistor_temperature_C"
    ])

  return fileName

def writeCSV(fileName):
  ## open file
  with open(fileName, 'a', newline='') as file:
    writer = csv.writer(file)

    ## create timestamp
    now = datetime.now()
    timestamp = datetime.timestamp(now)

    ## thermistor conversions
    if chan0.voltage != 0:
        thermistor_r = therm_r2 * (therm_v_in/chan0.voltage - 1)
        thermistor_temp = (1/(therm_a + therm_b * math.log(thermistor_r) + therm_c * pow(math.log(thermistor_r), 3))) - 273
    else:
        thermistor_r = None
        thermistor_temp = None


    ## write line
    writer.writerow([
      ## timestamp
      timestamp,
      ## am2320 (Humidity Sensor)
      am2320.relative_humidity,
      am2320.temperature,
      ## bmp388 (Barometric Pressure Sensor)
      bmp388.temperature,
      bmp388.pressure,
      bmp388.altitude,
      ## bno055 (IMU)
      bno055.acceleration,
      bno055.euler,
      bno055.gravity,
      bno055.gyro,
      bno055.linear_acceleration,
      bno055.magnetic,
      bno055.temperature,
      bno055.quaternion,
      ## mcp3008 (Thermistor)
      chan0.value,
      chan0.voltage,
      thermistor_r,
      thermistor_temp
    ])

  ## sleep for 5 seconds
  threading.Timer(5.0, writeCSV, [fileName]).start()

  return None

## intitialize creation
fileName = createCSV()

## loop: five second iteration to write csv
writeCSV(fileName)