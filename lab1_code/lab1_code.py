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

f = open("lab1_output.txt","w+")

while True:

    f.write("Output from humidity sensor:")
    f.write("Temperature: ", am2320.temperature)
    f.write("Humidity: ", am2320.relative_humidity)

    f.write("\nOutput from IMU:")
    f.write('Temperature: {} degrees C'.format(bno055.temperature))
    f.write('Accelerometer (m/s^2): {}'.format(bno055.acceleration))
    f.write('Magnetometer (microteslas): {}'.format(bno055.magnetic))
    f.write('Gyroscope (rad/sec): {}'.format(bno055.gyro))
    f.write('Euler angle: {}'.format(bno055.euler))
    f.write('Quaternion: {}'.format(bno055.quaternion))
    f.write('Linear acceleration (m/s^2): {}'.format(bno055.linear_acceleration))
    f.write('Gravity (m/s^2): {}'.format(bno055.gravity))

    f.write("\nOutput from IMU:")
    f.write("Pressure: ", bmp388.pressure)
    f.write("Temperature: \n", bmp388.temperature)

    print("Output from humidity sensor:")
    print("Temperature: ", am2320.temperature)
    print("Humidity: ", am2320.relative_humidity)

    print("\nOutput from IMU:")
    print('Temperature: {} degrees C'.format(bno055.temperature))
    print('Accelerometer (m/s^2): {}'.format(bno055.acceleration))
    print('Magnetometer (microteslas): {}'.format(bno055.magnetic))
    print('Gyroscope (rad/sec): {}'.format(bno055.gyro))
    print('Euler angle: {}'.format(bno055.euler))
    print('Quaternion: {}'.format(bno055.quaternion))
    print('Linear acceleration (m/s^2): {}'.format(bno055.linear_acceleration))
    print('Gravity (m/s^2): {}'.format(bno055.gravity))

    print("\nOutput from IMU:")
    print("Pressure: ", bmp388.pressure)
    print("Temperature: \n", bmp388.temperature)

    time.sleep(2)


f.close()