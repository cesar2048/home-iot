#ifndef ESP32IO_H_
#define ESP32IO_H_

#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <DHT_U.h>
#include <DHT.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>

#include <Preferences.h>
#include "Application.hpp"

// ESP hardware
#define LED_BUILTIN   2  // DevKit Do-It
#define DHTTYPE       DHT22
#define PREFS_RW_MODE false
#define PREFS_RO_MODE true

// Adafruit NeoPixel
#define COLOR_RED   0x00FF00 // red
#define COLOR_GREEN 0xFF0000 // green
#define COLOR_BLUE  0x0000FF // blue
#define COLOR_OFF   0

#define WAKEUP_STATE  1
#if defined(NEOPIXEL_POWER)
    // Adafruit QT
    #define DHTPIN        35
    #define WAKEUP_PIN    GPIO_NUM_16 // RX Pin
#else
    // Devkit Do-it
    #define DHTPIN        15 
    #define WAKEUP_PIN    GPIO_NUM_4
#endif

// influxdb
#define TZ_INFO "UTC-6"

extern const char *baseAPName;

class ESP32Adapter : public IOAdapter {
    DHT_Unified *dht;

public:
    ESP32Adapter();
    void init();
    int read_state();
    void set_state(int state, bool restart = false);    
    
    void blink_to_show(int message);
    void restart();
    void init_sensors();

    DataReading read_temperature();
    DataReading read_humidity();

    void deepSleep(int milliSeconds);
    int isWakeUpButtonOn();

private:
    void statusLed(int color);
    int statusLedColor;
};

// ------------------------- Wifi section -----------------

#if ROLE == ROLE_WIFI
    class ESP32Wifi : public WiFiAdapter {
        WiFiServer server;
        WiFiMulti *wifiMulti;
        void handle_request(WiFiClient &client, String &method, String &url, String &body);

    public:
        void start_AP_server();
        void handle_client();
        bool start_wifi_client();
        bool send_measurements_to_influx_server(float temperature, float humidity);
    };
#else 
    class DummyWifi : public WiFiAdapter {
    public:
        void start_AP_server();
        void handle_client();
        bool start_wifi_client();
        bool send_measurements_to_influx_server(float temperature, float humidity);
    };
#endif

// ------------------------- bluetooth section -----------------

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ESPBTAdapter : public BTAdapter {
public:
    ESPBTAdapter();
    void startAdvertising(std::string deviceName);
    void setTemperature(float value);
    void setHumidity(float value);
    bool clientIsDone();

    bool clientWroteSomething;

private:
    BLEServer* pServer;
    BLEService *pService;
    BLECharacteristic* pCharacteristic;

    float temp, humidity;
};


class CharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(ESPBTAdapter *btAdapter);
    void onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param);
private:
    ESPBTAdapter *btAdapter;
};

#endif /* ESP32IO_H_ */