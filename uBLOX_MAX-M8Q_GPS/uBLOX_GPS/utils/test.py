#!/usr/bin/python
import RPi.GPIO as GPIO
import sys
import time

GPIO.setmode(GPIO.BCM) # Broadcom pin-numbering scheme

if len(sys.argv) == 1:
  print "no arguments"
else:
  pin = int(sys.argv[1])
  GPIO.setup(pin, GPIO.OUT)
  try:
    while True:
        GPIO.output(pin, GPIO.LOW)
        time.sleep(1)
        GPIO.output(pin, GPIO.HIGH)
        time.sleep(1)
  except KeyboardInterrupt: # If CTRL+C is pressed, exit cleanly:
    GPIO.cleanup() # cleanup all GPIO
    print "done"
