#ifndef ESP32IO_H_
#define ESP32IO_H_

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <DHT.h>

#include <Preferences.h>
#include "Application.hpp"

// esp hardware
#define LED_BUILTIN 2
#define DHTPIN      14
#define DHTTYPE     DHT22

#define PREFS_RW_MODE false
#define PREFS_RO_MODE true

// for influxdb
#define TZ_INFO "UTC-6"


// const char *ssid = "SupernovaIoT";
extern const char *ssid;

class ESP32Adapter : public IOAdapter {
    WiFiServer server;
    WiFiMulti *wifiMulti;
    DHT_Unified *dht;

    void handle_request(WiFiClient &client, String &method, String &url, String &body);

public:
    ESP32Adapter();

    int read_state();
    void set_state(int state);
    void start_AP_server();
    void handle_client();
    bool start_wifi_client();
    void blink_to_show(int message);
    void restart();
    void init_sensors();

    DataReading read_temperature();
    DataReading read_humidity();
    bool send_measurements_to_influx_server(float temperature, float humidity);
};

#endif /* ESP32IO_H_ */