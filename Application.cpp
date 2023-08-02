#include "Application.hpp"
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>

/*
# Application overview

## Pseudocode

state = read_app_state()
if state == 'init':

    start_in_AP_mode()

    while(true):
        client = wait_for_client()
        request = read_client_request()
        if request == 'set_wifi_credentials':
            preferences.store('wifi_credentials', credentials)
            set_app_state('configured')
            restart()

if state == 'configured':
    credentials = preferences.read('wifi_credentials')
    result = wifi_connect_using(credentials)
    if result == 'failure':
        preferences.delete('wifi_credentials')
        set_app_state('init')

        blink_to_show('wifi failed')
        restart()

    else:
        point = make_measurement()
        aggregate_data(point)

        if point_count() == target_count:
            send_data_to_influx()
*/ 

Application::Application(IOAdapter *adapterInstance):adapter(adapterInstance) {
}

void Application::setup() {
    IOAdapter *a = this->adapter;
    if (a->read_state() == APP_INIT) {
        a->start_AP_server();
    } else if (a->read_state() == APP_CONFIGURED) {
        bool connect_success = a->start_wifi_client();
        if (!connect_success) {
            a->blink_to_show(MESSAGE_FAILED_TO_CONNECT);
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
            return;
        }
        
        bool send_success = a->send_measurements_to_influx_server(temperature, humidity);
        if (!send_success) {
            a->blink_to_show(MESSAGE_FAILED_TO_WRITE);
        }
    }
}
