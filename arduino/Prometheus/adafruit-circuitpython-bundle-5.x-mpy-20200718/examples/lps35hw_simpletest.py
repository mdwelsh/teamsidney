import time
import board
import adafruit_lps35hw

i2c = board.I2C()
lps = adafruit_lps35hw.LPS35HW(i2c)

while True:
    print("Pressure: %.2f hPa" % lps.pressure)
    print("Temperature: %.2f C" % lps.temperature)
    print("")
    time.sleep(1)
