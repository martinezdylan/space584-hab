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
cs = digitalio.DigitalInOut(board.D22)

# create the mcp object
mcp = MCP.MCP3008(spi, cs)

# create an analog input channel on pin 0
chan0 = AnalogIn(mcp, MCP.P0)

print("Raw ADC Value: ", chan0.value)
print("ADC Voltage: " + str(chan0.voltage) + "V")

last_read = 0       # this keeps track of the last potentiometer value
tolerance = 250     # to keep from being jittery we'll only change
                    # volume when the pot has moved a significant amount
                    # on a 16-bit ADC

A = 0.001468
B = 0.0002383
C = 0.0000001007

def remap_range(value, left_min, left_max, right_min, right_max):
    # this remaps a value from original (left) range to new (right) range
    # Figure out how 'wide' each range is
    left_span = left_max - left_min
    right_span = right_max - right_min

    # Convert the left range into a 0-1 range (int)
    valueScaled = int(value - left_min) / int(left_span)

    # Convert the 0-1 range into a value in the right range.
    return int(right_min + (valueScaled * right_span))

while True:
    # we'll assume that the pot didn't move
    trim_pot_changed = False

    # read the analog pin
    trim_pot = chan0.value

    # how much has it changed since the last read?
    pot_adjust = abs(trim_pot - last_read)

    if pot_adjust > tolerance:
        trim_pot_changed = True

    if trim_pot_changed:
        # convert 16bit adc0 (0-65535) trim pot read into 0-100 volume level
        set_volume = remap_range(trim_pot, 0, 65535, 0, 100)

        # set OS volume playback volume
        print("Volume = {volume}%" .format(volume = set_volume))
        set_vol_cmd = "sudo amixer cset numid=1 -- {volume}% > /dev/null" \
        .format(volume = set_volume)
        os.system(set_vol_cmd)

        # save the potentiometer reading for the next loop
        last_read = trim_pot

		temp = 1 / ( A + B * math.log(R) + C * pow(math.log(R), 3) ) #this is right for converting resistant/A/B/C to temp
		temp = temp - 273 #may not want this to be automatically converted
		print("Temperature from thermistor: ", temp) #will need to make this write to a file at some point, but this is enough to show us it's working

    # hang out and do nothing for a half second
    time.sleep(0.5)



'''
GPIO.setmode(GPIO.BCM)
#OR GPIO.setmode(GPIO.BOARD) #if the above code doesn't work try this

GPIO.cleanup() #clears the pin values
GPIO.setup(0,GPIO.IN) #sets 0 as gpis in to take in the data on that pin

GPIO.setup(___,GPIO.OUT) #can set it to high or low power high-3.3V, low - OV with GPIO.output(___, GPIO.HIGH)

input = GPIO.input(0)

print(GPIO.input(0))

#defined by the data sheet for the thermistor
'''

while True: #will need to edit how long this loop will run, may be able to manually cut it off when it lands. 
	R = input / 0.0001  #resistance is voltage divided by current, maybe we can use power instead of current. 
	temp = 1 / ( A + B * math.log(R) + C * pow(math.log(R), 3) ) #this is right for converting resistant/A/B/C to temp
	temp = temp - 273 #may not want this to be automatically converted
	print(temp) #will need to make this write to a file at some point, but this is enough to show us it's working

#GPIO.cleanup() #clears the pin values