# Overview

# Features
  * "Configuration mode" starts an AP with a Web server to set parameters
    LED blinks every 5 seconds in this mode
    AP shows as "Hyg00FF" where the last digits are from the mac address
    Configuration page is available at http://192.168.2.1/
    Indicators:
      * LED blink twice: Failed to connect to WiFi
  * "Data aquisition mode" pushes a a new measurement every 1 minute
    Indicators:
      * LED blinks 4 times: Failed to write to InfluxDB
      * LED blinks 3 times: Failed to read from sensor

# References

ESP32 Arduino:
  * [Pinout: ESP32 DevKit 36 & 30 pin](https://www.etechnophiles.com/esp32-dev-board-pinout-specifications-datasheet-and-schematic/)
  * [Pinout: (doit_esp32_devkit-v1_30p) Github/TronixLab](https://github.com/TronixLab/DOIT_ESP32_DevKit-v1_30P)
  * [Pinout: (doit-esp32-devkit-v1) circuitstate.com](https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)
  * [Pinout: (AdafruitQTPy)](https://learn.adafruit.com/adafruit-qt-py-esp32-s2/pinouts)
  * [Examples: WiFi Client](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClient/WiFiClient.ino)
  * [Examples: AdafuitQT Pins](https://github.com/espressif/arduino-esp32/blob/master/variants/adafruit_qtpy_esp32s2/pins_arduino.h)

ESP32 Tutorials:
  * [ESP32 supress boot messages](https://community.platformio.org/t/esp32-prevent-from-sending-serial-output-at-startup/18756/2)
  * [Preferences](https://github.com/espressif/arduino-esp32/blob/21b88659b9ded3fcc1082f23f498bc2a04cd4f1b/docs/source/tutorials/preferences.rst#L668)
  * [NodeMCU with DHT22](https://simple-circuit.com/esp-12e-nodemcu-ssd1306-dht22-am2302/)

ESP32 DeepSleep:
  * [Deep sleep power consumption](https://www.reddit.com/r/esp32/comments/11yjvvk/deep_sleep_power_consumption_esp32s3_way_too_high/)
  * [Esp32 FireBeetle deep sleep](https://lucidar.me/en/esp32/power-consumption-of-esp32-firebeetle-dfr0478/)
  * [Esp32 DevKitC power consumption](https://lucidar.me/en/esp32/power-consumption-of-esp32-devkitc-v4/)
  * [Wake up from Deep Sleep](https://randomnerdtutorials.com/esp32-external-wake-up-deep-sleep/)
  * [Reading TX w/o USB](https://mischianti.org/2021/03/10/esp32-power-saving-modem-and-light-sleep-2/)

PCBs:
  * [DIY PCB with Toner printer](https://hackaday.com/2016/09/12/take-your-pcbs-from-good-to-great-toner-transfer/)
  * [KiCad tutorial](https://www.youtube.com/watch?v=vaCVh2SAZY4&list=PL3bNyZYHcRSUhUXUt51W6nKvxx2ORvUQB&index=1&pp=iAQB)

Other sources for GPIO reference:

