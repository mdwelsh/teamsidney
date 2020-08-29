import adafruit_trellism4

trellis = adafruit_trellism4.TrellisM4Express()

while True:
    pressed = trellis.pressed_keys
    if pressed:
        print("Pressed:", pressed)
