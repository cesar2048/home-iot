/**************************************************************************************
 ESP8266 NodeMCU interfacing with SSD1306 OLED and DHT22 (AM2302) sensor
***************************************************************************************/
//#include <Adafruit_Sensor.h>
//#include <DHT_U.h>
//#include <Wire.h>
//#include <DHT.h>
//#include <InfluxDbClient.h>
//#include <InfluxDbCloud.h>

#include "EspAdapter.hpp"
#include "Application.hpp"
#include "SmoothSignal.hpp"
// #include "env.h"

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
bool doBlinky = false;

void setup() {
    // all debug code
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(WAKEUP_PIN,  INPUT);
    pinMode(DEBUG_PIN,   INPUT);
    
    bool doDebug   = digitalRead(DEBUG_PIN);
    bool wakeUpBtn = digitalRead(WAKEUP_PIN);
    if (doDebug) {
        Serial.begin(115200, SERIAL_8N1, CUSTOM_RX, CUSTOM_TX);
        doBlinky = digitalRead(WAKEUP_PIN); // blinky if Debug && WakeUpBtn are ON
    } else {
        Serial.begin(115200);
    }
    
    #ifdef TINY_PICO
      Serial.printf("----------- STARTUP (TinyPico, debug:%i, wake:%i)\n", doDebug, wakeUpBtn);
    #else
      #ifdef NEOPIXEL_POWER
        Serial.printf("----------- STARTUP (QtPy, debug:%i, wake:%i)\n", doDebug, wakeUpBtn);
      #else
        Serial.printf("----------- STARTUP (Devkit DoIt, debug:%i, wake:%i)\n", doDebug, wakeUpBtn);
      #endif
    #endif

    if (doDebug) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1500);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (!doBlinky) {
      main_app = new Application(&adapter, &btAdapter, &wifiAdapter, &sensors);
      main_app->setup();
    }
}

void loop() {
    if (doBlinky) {
      bool pushBtnUp = digitalRead(DEBUG_PIN);
      int delayMs    = pushBtnUp ? 50 : 1000;
      Serial.printf("BLINK: Push: %i\n", pushBtnUp);

      digitalWrite(LED_BUILTIN, HIGH);
      delay(delayMs);
      digitalWrite(LED_BUILTIN, LOW);
      delay(delayMs);
    } else {
      // Serial.printf("MAIN\n"); delay(500);
      main_app->loop();
    }
}
