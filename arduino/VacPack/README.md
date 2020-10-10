MicroPython on ESP32:
http://docs.micropython.org/en/v1.10/esp32/tutorial/intro.html

Firmware:
https://micropython.org/download/esp32/

```bash
pip install esptool
esptool.py --port /dev/tty.SLAB_USBtoUART erase_flash
esptool.py --chip esp32 --port /dev/tty.SLAB_USBtoUART write_flash -z 0x1000 esp32-idf3-20191220-v1.12.bin
brew install picocom
picocom /dev/tty.SLAB_USBtoUART -b115200

>>> import webrepl_setup
>>> import network
>>> wlan = network.WLAN(network.STA_IF)
>>> wlan.active(True)
>>> wlan.connect('theonet', 'j********')
>>> webrepl.stop()
>>> webrepl.start()
```

Visit:
http://micropython.org/webrepl/#192.168.86.152:8266


