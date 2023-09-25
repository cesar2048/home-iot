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
    // all debug code
    pinMode(DEBUG_PIN, OUTPUT);
    pinMode(WAKEUP_PIN, INPUT);
    digitalWrite(DEBUG_PIN, HIGH);
    Serial.begin(115200);
    int pinStatus = digitalRead(WAKEUP_PIN);
    if (pinStatus) {
        delay(1500);
    }
    digitalWrite(DEBUG_PIN, LOW);
    
    #ifdef TINY_PICO
      Serial.printf("STARTUP (TinyPico, debug:%i, wake:%i)\n", DEBUG_PIN, WAKEUP_PIN);
    #else
      #ifdef NEOPIXEL_POWER
        Serial.printf("STARTUP (QtPy, debug:%i, wake:%i)\n", DEBUG_PIN, WAKEUP_PIN);
      #else
        Serial.printf("STARTUP (Devkit DoIt, debug:%i, wake:%i)\n", DEBUG_PIN, WAKEUP_PIN);
      #endif
    #endif

    main_app = new Application(&adapter, &btAdapter, &wifiAdapter, &sensors);
    main_app->setup();
}

void loop() {
    main_app->loop();
}
