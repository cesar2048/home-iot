# Overview

# Features
  * "Configuration mode" starts an AP with a Web server to set parameters
    LED blinks every 5 seconds in this mode
    AP shows as "SupernovaIoT", configuration page is available at http://192.168.1.1/
    Indicators:
      * LED blink twice: Failed to connect to WiFi
  * "Data aquisition mode" pushes a a new measurement every 1 minute
    Indicators:
      * LED blinks 4 times: Failed to write to InfluxDB
      * LED blinks 3 times: Failed to read from sensor

# References
  * [ESP32 WiFi Client](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClient/WiFiClient.ino)
  * [ESP32 DevKit 36 & 30 pin](https://www.etechnophiles.com/esp32-dev-board-pinout-specifications-datasheet-and-schematic/)
  * [ESP32 supress boot messages](https://community.platformio.org/t/esp32-prevent-from-sending-serial-output-at-startup/18756/2)
  * [NodeMCU with DHT22](https://simple-circuit.com/esp-12e-nodemcu-ssd1306-dht22-am2302/)

Other sources for GPIO reference:
  * [github.com/TronixLab/DOIT_ESP32_DevKit-V1_30P](https://github.com/TronixLab/DOIT_ESP32_DevKit-v1_30P)
  * [circuitstate.com/pinouts/doit-esp32-devkit-v1](https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)

DeepSleep power ESP32:
  * [Deep sleep power consumption](https://www.reddit.com/r/esp32/comments/11yjvvk/deep_sleep_power_consumption_esp32s3_way_too_high/)
  * [Esp32 FireBeetle deep sleep](https://lucidar.me/en/esp32/power-consumption-of-esp32-firebeetle-dfr0478/)
  * [Esp32 DevKitC power consumption](https://lucidar.me/en/esp32/power-consumption-of-esp32-devkitc-v4/)

  [Preferences](https://github.com/espressif/arduino-esp32/blob/21b88659b9ded3fcc1082f23f498bc2a04cd4f1b/docs/source/tutorials/preferences.rst#L668)