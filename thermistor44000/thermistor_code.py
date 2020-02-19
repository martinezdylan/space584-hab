import os
import time
import busio
import digitalio
import board
import adafruit_mcp3xxx.mcp3008 as MCP
import RPi.GPIO as GPIO
import math
from adafruit_mcp3xxx.analog_in import AnalogIn

# create the spi bus
spi = busio.SPI(clock=board.SCK, MISO=board.MISO, MOSI=board.MOSI)

# create the cs (chip select)
cs = digitalio.DigitalInOut(board.D5)

# create the mcp object
mcp = MCP.MCP3008(spi, cs)

# create an analog input channel on pin 0
chan0 = AnalogIn(mcp, MCP.P0)

# conversion coefficients
A = 0.001468
B = 0.0002383
C = 0.0000001007

# supply voltage
Vin = 3.3

# resistor in circuit
R2 = 10e3

while True:
     
    print("Raw ADC Value: ", chan0.value)
    print("ADC Voltage: " + str(chan0.voltage) + " V")

    Vout = chan0.voltage
    
    if Vout != 0:
        
        thermistor_R = R2 * (Vin/Vout - 1)
        
        print("Resistance from thermistor: ", thermistor_R)

        # converting resistant/A/B/C to temp
        temp = 1 / (A + B * math.log(thermistor_R) + C * pow(math.log(thermistor_R ), 3))
        
        temp = temp - 273 # may not want this to be automatically converted
        
        print("Temperature from thermistor: ", str(temp) + " C\n") # will need to make this write to a file at some point, but this is enough to show us it's working

    # hang out and do nothing for a second
    time.sleep(1)


'''
GPIO.setmode(GPIO.BCM)
#OR GPIO.setmode(GPIO.BOARD) #if the above code doesn't work try this

GPIO.cleanup() #clears the pin values
GPIO.setup(0,GPIO.IN) #sets 0 as gpis in to take in the data on that pin

GPIO.setup(___,GPIO.OUT) #can set it to high or low power high-3.3V, low - OV with GPIO.output(___, GPIO.HIGH)

input = GPIO.input(0)

print(GPIO.input(0))

#defined by the data sheet for the thermistor


while True: #will need to edit how long this loop will run, may be able to manually cut it off when it lands. 
	R = input / 0.0001  #resistance is voltage divided by current, maybe we can use power instead of current. 
	temp = 1 / ( A + B * math.log(R) + C * pow(math.log(R), 3) ) #this is right for converting resistant/A/B/C to temp
	temp = temp - 273 #may not want this to be automatically converted
	print(temp) #will need to make this write to a file at some point, but this is enough to show us it's working

#GPIO.cleanup() #clears the pin values
'''
