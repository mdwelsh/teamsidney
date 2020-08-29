import time
import board
import busio
import adafruit_mcp9600

# frequency must be set for the MCP9600 to function.
# If you experience I/O errors, try changing the frequency.
i2c = busio.I2C(board.SCL, board.SDA, frequency=100000)
mcp = adafruit_mcp9600.MCP9600(i2c)

while True:
    print((mcp.ambient_temperature, mcp.temperature, mcp.delta_temperature))
    time.sleep(1)
