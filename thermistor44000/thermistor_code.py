import RPi.GPIO as GPIO
import math

GPIO.setmode(GPIO.BCM)
#OR GPIO.setmode(GPIO.BOARD) #if the above code doesn't work try this

GPIO.cleanup() #clears the pin values
GPIO.setup(0,GPIO.IN) #sets 0 as gpis in to take in the data on that pin

GPIO.setup(___,GPIO.OUT) #can set it to high or low power high-3.3V, low - OV with GPIO.output(___, GPIO.HIGH)

input = GPIO.input(0)

print(GPIO.input(0))

#defined by the data sheet for the thermistor
A = 0.001468
B = 0.0002383
C = 0.0000001007

while True: #will need to edit how long this loop will run, may be able to manually cut it off when it lands. 
	R = input / 0.0001  #resistance is voltage divided by current, maybe we can use power instead of current. 
	temp = 1 / ( A + B * math.log(R) + C * pow(math.log(R), 3) ) #this is right for converting resistant/A/B/C to temp
	temp = temp - 273 #may not want this to be automatically converted
	print(temp) #will need to make this write to a file at some point, but this is enough to show us it's working

GPIO.cleanup() #clears the pin values