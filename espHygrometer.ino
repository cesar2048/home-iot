/**************************************************************************************
 ESP8266 NodeMCU interfacing with SSD1306 OLED and DHT22 (AM2302) sensor
***************************************************************************************/
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "EspAdapter.hpp"
#include "Application.hpp"
#include "SmoothSignal.hpp"
#include "env.h"

ESP32Adapter adapter;
Application *main_app;

void setup() {
    Serial.begin(115200);
    delay(1500);

    main_app = new Application(&adapter);
    main_app->setup();
}

void loop() {
    main_app->loop();
}
