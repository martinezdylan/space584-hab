import time
import board
import busio
import adafruit_am2320
import adafruit_bno055
import adafruit_bmp3xx

i2c = busio.I2C(board.SCL, board.SDA)
am2320 = adafruit_am2320.AM2320(i2c)
bno055 = adafruit_bno055.BNO055(i2c)
bmp388 = adafruit_bmp3xx.BMP3XX_I2C(i2c)

while True:
    print("Output from humidity sensor:")
    print("Temperature: ", am2320.temperature)
    print("Humidity: ", am2320.relative_humidity)

    print("Output from IMU:")
    print('Temperature: {} degrees C'.format(sensor.temperature))
    print('Accelerometer (m/s^2): {}'.format(sensor.acceleration))
    print('Magnetometer (microteslas): {}'.format(sensor.magnetic))
    print('Gyroscope (rad/sec): {}'.format(sensor.gyro))
    print('Euler angle: {}'.format(sensor.euler))
    print('Quaternion: {}'.format(sensor.quaternion))
    print('Linear acceleration (m/s^2): {}'.format(sensor.linear_acceleration))
    print('Gravity (m/s^2): {}'.format(sensor.gravity))

    print("Output from IMU:")
    print("Pressure: {:6.1f}  Temperature: {:5.2f}".format(bmp.pressure, bmp.temperature))

    time.sleep(2)
