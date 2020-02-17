# Importing modules
import spidev # To communicate with SPI devices
from time import sleep  # To add delay
# Start SPI connection

spi = spidev.SpiDev() # Created an object
spi.open(0,0)

# Read MCP3008 data
def analogInput(channel):
  spi.max_speed_hz = 1350000
  adc = spi.xfer2([1,(8+channel)<<4,0])
  data = ((adc[1]&3) << 8) + adc[2]
  return data

# Below function will convert data to voltage
def Volts(data):
  volts = (data * 3.3) / float(1023)
  volts = round(volts, 2) # Round off to 2 decimal places
  return volts
 
# Below function will convert data to temperature.
def Temp(data):
  temp = ((data * 330)/float(1023))-50
  temp = round(temp)
  return temp

while True:
  temp_output = analogInput(0) # Reading from CH0
  temp_volts = Volts(temp_output)
  temp       = Temp(temp_output)
 
  print("Temp : {} ({}V) {} deg C".format(temp_output,temp_volts,temp))
  sleep(5)
  