Bugs / observations:

* Strandtest with Neopixel
  - Brightness 255 is fine.
  - Anything below 255 shows spurious pixels, both when black and when
    LEDs are lit.
  - Doing a colorWipe() at speed 0 results in a lot of glitching, even
    at brightness 255.
  - So, stable only when brightness is 255 and not wiping too fast.
  - Using 5V signals vs 3.3V signals has no effect on this.
  - Using external power supply is the same.

* Bug in the setBrightness code perhaps?

* Blinky with Neopixel
  - Keeping brightness at 255 results in a fair bit of glitching for
    modes like strobe. Not quite the same glitching as when using
    strandtest though.
  - Generally animations seem to be a bit choppy.
  - For whatever reason, external power supply is *not* working with
    this program. Maybe due to UART problem, or use of WiFi chip?

To try:
  - Disable Serial to see if that helps.
  - Rethink task model -- disable concurrency?
  - Check to see if there is a bug in setBrightness code in Neopixel
    library.

