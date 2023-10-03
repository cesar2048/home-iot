#ifndef ESP32IO_H_
#define ESP32IO_H_

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Adafruit_AHTX0.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <TinyPICO.h>

#include "Application.hpp"

// Define for TinyPico
// #define TINY_PICO 1

// constants
#define DHTTYPE       DHT22
#define PREFS_RW_MODE false
#define PREFS_RO_MODE true
#define WAKEUP_STATE  1
#define TZ_INFO       "UTC-6" // influxdb

// Colors (defaults for TinyPico)
#define COLOR_RED     0x00FF0000  // red
#define COLOR_GREEN   0x0000FF00  // green
#define COLOR_BLUE    0x000000FF  // blue
#define COLOR_OFF     0

// IO pins (defaults for ESP32 DevKit DoIt)
#define LED_BUILTIN   GPIO_NUM_2  // default led
#define DEBUG_PIN     GPIO_NUM_33 // toggl switch
#define DHTPIN        GPIO_NUM_15 // Dht22 sensor
#define WAKEUP_PIN    GPIO_NUM_26 // push button
#define CUSTOM_TX     -1  // default
#define CUSTOM_RX     -1  // default

#ifdef TINY_PICO
  // overrides: For TinyPico
  #define LED_BUILTIN   GPIO_NUM_27 // default led
  #define DEBUG_PIN     GPIO_NUM_25 // toggl switch
  #define DHTPIN        GPIO_NUM_4  // Dht22 sensor
  #define WAKEUP_PIN    GPIO_NUM_26 // push button
  #define CUSTOM_TX     GPIO_NUM_14
  #define CUSTOM_RX     GPIO_NUM_15
  #define COLOR_RAINBOW 0xFF444444  // rainbow
#else
  #ifdef NEOPIXEL_POWER
    // overrides: For QtPy
    #define LED_BUILTIN   GPIO_NUM_13 // on led
    #define DEBUG_PIN     GPIO_NUM_25 // dbg switch
    #define WAKEUP_PIN    GPIO_NUM_27 // push button
    #define CUSTOM_TX     GPIO_NUM_32
    #define CUSTOM_RX     GPIO_NUM_7
    #define COLOR_RED     0x0000FF00  // red
    #define COLOR_GREEN   0x00FF0000  // green
  #endif
#endif


extern const char *baseAPName;

class ESP32Adapter : public IOAdapter {
public:
    ESP32Adapter();
    void init();
    int read_state();
    void set_state(int state, bool restart = false);    
    
    void blink_to_show(int message);
    void restart();
    
    void deepSleep(int milliSeconds);
    int isWakeUpButtonOn();

private:
    void statusLed(int color);
    int statusLedColor;
};

// ------------------------- Sensors section -----------------

class BTSensorProvider : public SensorProvider {
    BLERemoteCharacteristic* pRemoteCharacteristic;
    BLEClient*               pClient;
    bool connect();
public:
    BLEAdvertisedDevice*     myDevice;
    bool doScan;
    bool init();
    bool readValues(float *temp, float *humid);
};
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    BTSensorProvider *btSensor;
    void onResult(BLEAdvertisedDevice advertisedDevice);
public:
    MyAdvertisedDeviceCallbacks(BTSensorProvider *btSensor);
};





class DHTSensorProvider : public SensorProvider {
    Adafruit_AHTX0 aht;
    bool initialized;    
public:
    bool init();
    bool readValues(float *temp, float *humid);
};


// ------------------------- Wifi section -----------------

#if ROLE == ROLE_WIFI || ROLE == ROLE_BLE_CLIENT
    class ESP32Wifi : public WiFiAdapter {
        WiFiServer server;
        WiFiMulti *wifiMulti;
        void handle_request(WiFiClient &client, String &method, String &url, String &body);

    public:
        ESP32Wifi();
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

class ESPBTAdapter : public BTServer {
public:
    ESPBTAdapter();
    void startAdvertising(std::string deviceName);
    void setvalues(float temp, float humid);
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