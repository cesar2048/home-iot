#include "Application.hpp"
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>

Application::Application(IOAdapter *adapterInstance):adapter(adapterInstance) {
}

void Application::setup() {
    IOAdapter *a = this->adapter;
    if (a->read_state() == APP_INIT) {
        Serial.print("Start in APP_INIT");
        a->start_AP_server();

    } else if (a->read_state() == APP_CONFIGURED) {
        Serial.println("Start as APP_CONFIGURED");
        bool wifiConnected = a->start_wifi_client();
        if (!wifiConnected) {
            Serial.println("failed to connect to wifi");
            a->blink_to_show(MESSAGE_FAILED_TO_CONNECT);
            a->set_state(APP_INIT);
            a->restart();
        }
    }
}

void Application::loop() {
    IOAdapter *a = this->adapter;
    if (a->read_state() == APP_INIT) {
        a->handle_client();

    } else if (a->read_state() == APP_CONFIGURED) {
        float temperature = a->read_temperature();
        float humidity    = a->read_humidity();
        if (isnan(temperature) || isnan(humidity)) {
            a->blink_to_show(MESSAGE_FAILED_TO_READ);
            delay(1500);
            return;
        }
        
        bool send_success = a->send_measurements_to_influx_server(temperature, humidity);
        if (!send_success) {
            a->blink_to_show(MESSAGE_FAILED_TO_WRITE);
            delay(1500);
        }
    }
}
