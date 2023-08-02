#ifndef ESP32IO_H_
#define ESP32IO_H_

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "Application.hpp"

#define LED_BUILTIN 2

#define RW_MODE false
#define RO_MODE true

// const char *ssid = "SupernovaIoT";
extern const char *ssid;

class ESP32Adapter : public IOAdapter {
    WiFiServer server;

public:
    ESP32Adapter();

    int read_state();
    void start_AP_server();
    void handle_client();
    bool start_wifi_client();
    void blink_to_show(int message);
    void restart();

    float read_temperature();
    float read_humidity();
    bool send_measurements_to_influx_server(float temperature, float humidity);
};

#endif /* ESP32IO_H_ */