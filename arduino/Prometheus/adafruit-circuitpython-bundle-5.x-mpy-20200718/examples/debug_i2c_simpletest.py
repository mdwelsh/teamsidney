"""This example uses the LIS3DH accelerometer. Debug_I2C can be used with any I2C device."""
import busio
import board
import digitalio
from adafruit_debug_i2c import DebugI2C
import adafruit_lis3dh

i2c = DebugI2C(busio.I2C(board.SCL, board.SDA))
int1 = digitalio.DigitalInOut(board.ACCELEROMETER_INTERRUPT)
accelerometer = adafruit_lis3dh.LIS3DH_I2C(i2c, address=0x19, int1=int1)

print(accelerometer.acceleration)

for i in range(2):
    print(accelerometer.acceleration)
