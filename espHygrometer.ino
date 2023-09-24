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
ESPBTAdapter btAdapter;

#if ROLE == ROLE_WIFI || ROLE == ROLE_BLE_CLIENT
    ESP32Wifi wifiAdapter;
#else
    DummyWifi wifiAdapter;
#endif

#if ROLE == ROLE_BLE_CLIENT
    BTSensorProvider sensors;
#else
    DHTSensorProvider sensors;
#endif

Application *main_app;

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println("STARTUP");
    main_app = new Application(&adapter, &btAdapter, &wifiAdapter, &sensors);
    main_app->setup();
}

void loop() {
    main_app->loop();
}
