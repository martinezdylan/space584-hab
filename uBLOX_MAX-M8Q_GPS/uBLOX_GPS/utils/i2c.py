#!/usr/bin/python
import RPi.GPIO as GPIO
from time import sleep

class BitbangI2C:
  """A bit banging I2C interface"""
  def __init__(self):
    self.sda = 2
    self.scl = 3
    self.delay = 0.01
    self.timeout = 100
    self.failed = False   
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False) 

  def open(self, addr):
    self.address = addr
    GPIO.setup(self.sda, GPIO.OUT)
    GPIO.setup(self.scl, GPIO.OUT)
    GPIO.output(self.sda, GPIO.LOW)
    GPIO.output(self.scl, GPIO.LOW)
    return False

  def puts(self, s):
    self.send_start()
    self.send_bits(self.address*2)
    ack = self.get_ack()
    for i in s:
        self.send_bits(i)
        ack = self.get_ack()
    self.send_stop()

  def getc(self):
    self.send_start()
    self.send_bits(self.address*2 + 1)
    self.get_ack()
    rv = self.read_bits()      
    self.send_nack() # ack = read more, nack = enougth
    self.send_stop()
    return rv

  def bit_delay(self):
    sleep(self.delay)
    #pass

  def send_start(self):
    # a HIGH to LOW transition on the SDA line while
    # SCL is HIGH indicateds a START condition
    self.set_clock_low()
    self.set_data_high()
    self.bit_delay()
    self.set_clock_high()
    self.bit_delay()
    self.set_data_low()
    self.bit_delay()

  def send_stop(self):
    # a LOW to HIGH transition on the SDA line while
    # SCL is HIGH defines a STOP condition
    self.set_clock_low()
    self.set_data_low()
    self.bit_delay()
    self.set_clock_high()
    self.bit_delay()
    self.set_data_high()
    self.bit_delay()
    self.set_clock_low()
       
  def send_ack(self):
    self.set_clock_low()
    self.set_data_low()
    self.bit_delay()
    self.set_clock_high()
    self.bit_delay()
    self.set_clock_low()
    self.bit_delay()         

  def send_nack(self):
    self.set_clock_low()
    self.set_data_high()
    self.bit_delay()
    self.set_clock_high()
    self.bit_delay()
    self.set_clock_low()
    self.bit_delay()

  def get_ack(self):
    GPIO.setup(self.sda, GPIO.IN, pull_up_down = GPIO.PUD_UP)
    self.set_clock_high()
    self.bit_delay()
    rc = GPIO.input(self.sda)
    self.set_clock_low()
    GPIO.setup(self.sda, GPIO.OUT)
    self.bit_delay()
    return (rc == 0)

  def send_bits(self, data):
    self.set_clock_low()
    # output data bit, MSB first
    for i in range(8):
      if data & 0x80 :    
        self.set_data_high()
      else:
        self.set_data_low()
      data <<= 1
      self.bit_delay()
      # generate clock
      self.set_clock_high()
      self.bit_delay()
      self.set_clock_low()
    self.bit_delay()

  def read_bits(self):
    val = 0
    self.set_clock_low()
    GPIO.setup(self.sda, GPIO.IN, pull_up_down = GPIO.PUD_UP)
    for i in range(8):
      self.bit_delay()
      self.set_clock_high()
      self.bit_delay()
      data = GPIO.input(self.sda)
      val = val << 1;
      val = val | data;
      self.set_clock_low()
    GPIO.setup(self.sda, GPIO.OUT)
    return (val)

  def set_clock_low(self):
    GPIO.output(self.scl, GPIO.LOW)

  def set_clock_high(self):
    GPIO.output(self.scl, GPIO.HIGH)

  def set_data_low(self):
    GPIO.output(self.sda, GPIO.LOW)

  def set_data_high(self):
    GPIO.output(self.sda, GPIO.HIGH)

setNav = [0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 
          0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 
          0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 
          0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC]
setTXT = [0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0xEB]
    
buffer = []
i2c = BitbangI2C()
i2c.open( 0x42)
i2c.puts(setTXT)

while True:
    c = i2c.getc()
    if c != 0xff:
       buffer.append(c)
    else:
       line = ''.join(chr(c) for c in buffer)
       print(line)
       buffer = []
       sleep(0.1)    
 