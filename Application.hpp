#ifndef APPLICATION_H_
#define APPLICATION_H_
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>
#include "SmoothSignal.hpp"

// application params
#define SMOOTHING_FACTOR   1 // TODO: conditionally make it a 4 for regular operation
#define MAX_WRITE_FAILURES 3

#define ROLE_WIFI       2 // performs the old logic
#define ROLE_BLE_SERVER 1 // reads from sensor, advertises, and gets read by client
#define ROLE_BLE_CLIENT 0 // scans for servers and reads value from client

#define ROLE            ROLE_BLE_SERVER

// constants
#define APP_INIT                    0
#define APP_TEST                    1
#define APP_CONFIGURED              2
#define APP_WAKEUP                  3

#define MESSAGE_FAILED_TO_CONNECT   1
#define MESSAGE_FAILED_TO_READ      2
#define MESSAGE_FAILED_TO_WRITE     3
#define MESSAGE_CONFIG_MODE_ENABLED 4
#define MESSAGE_READ                5
#define MESSAGE_BLE_SERVER          6
#define MESSAGE_DEMO                7

#if defined(NEOPIXEL_POWER)
  #define INDICATOR_LED 18  // AdaFruit QT-PY A0
#else
  #define INDICATOR_LED LED_BUILTIN // DevKit DoIt
#endif

struct DataReading { bool success; float value; };

class IOAdapter {
public:
    virtual void init() = 0;
    virtual int read_state() = 0;
    virtual void set_state(int, bool restart = false) = 0;
    virtual void blink_to_show(int message) = 0;
    virtual void restart() = 0;

    virtual void deepSleep(int milliSeconds) = 0;
    virtual int isWakeUpButtonOn() = 0;
};

class SensorProvider {
public:
    virtual bool init() =0;
    virtual bool readValues(float *temp, float *humid) =0;
};

class WiFiAdapter {
public:
    virtual void start_AP_server() = 0;
    virtual void handle_client() = 0;    
    virtual bool start_wifi_client() = 0;
    virtual bool send_measurements_to_influx_server(float temperature, float humidity) = 0;
};


class BTServer {
public:
    virtual void startAdvertising(std::string deviceName) =0;
    virtual void setvalues(float temp, float humid) =0;
    virtual bool clientIsDone() =0;
};


class Application {
    IOAdapter *adapter;
    BTServer *bt;
    WiFiAdapter *wifi;
    SensorProvider *sensor;

    bool readValues(float *temp, float *humi);
    bool cyclesDelay(int max, int delayMs);
public:
    Application(IOAdapter *adapterInstance, BTServer *btAdapter, WiFiAdapter *wifiAdapter, SensorProvider *sensor);
    void setup();
    void loop();
};

#endif /* APPLICATION_H_ */
