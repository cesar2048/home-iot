#include "Application.hpp"
#include <TinyPICO.h>

// RTC_DATA_ATTR int               failureWriteCount   = 0;
RTC_DATA_ATTR SignalAccumulator temperatureAcc = { 0, 0 };
RTC_DATA_ATTR SignalAccumulator humidityAcc    = { 0, 0 };
RTC_DATA_ATTR SmoothCounter     smoother       = { SMOOTHING_FACTOR, 0 };
RTC_DATA_ATTR int cycleCount = 0;
int cyclesOnServer           = 0;
bool performedRead           = false;
TinyPICO tp = TinyPICO();

Application::Application(IOAdapter *adapterInstance, BTAdapter *btInstance, WiFiAdapter *wifiAdapter):
    adapter(adapterInstance),
    bt(btInstance),
    wifi(wifiAdapter)
{
}

void Application::setup() {
    IOAdapter *a = this->adapter;
    a->init();
    #if ROLE == ROLE_BLE_SERVER
        if (a->isWakeUpButtonOn()) {
            if (a->read_state() == APP_INIT) {
                Serial.println("Change to APP_CONFIGURED");
                a->set_state(APP_CONFIGURED, true);
            } else {
                Serial.println("Change to APP_INIT");
                a->set_state(APP_INIT, true);
            }
        }
        a->init_sensors();
    #endif

    #if ROLE == ROLE_WIFI
        if (a->read_state() == APP_INIT) {
            Serial.println("Start in APP_INIT");
            a->start_AP_server();

        } else if (a->read_state() == APP_TEST) {
            Serial.println("Start as APP_TEST");

            bool wifiConnected = a->start_wifi_client();
            if (!wifiConnected) {
                Serial.println("failed to connect to wifi");
                a->blink_to_show(MESSAGE_FAILED_TO_CONNECT);
                a->set_state(APP_INIT, true);
            }

            a->set_state(APP_CONFIGURED, true);
        }

        if (a->read_state() == APP_CONFIGURED) {
            a->init_sensors();
        }
    #endif
}

void Application::loop() {
    IOAdapter *a = this->adapter;

    #if ROLE == ROLE_BLE_SERVER
        // ROLE SERVER
        if (a->read_state() == APP_INIT) {
            tp.DotStar_CycleColor(25);
            delay(100);
        } else if (a->read_state() == APP_CONFIGURED) {
            if (!performedRead) {
                DataReading temperature = a->read_temperature();
                DataReading humidity    = a->read_humidity();
                if (!temperature.success || !humidity.success) {
                    Serial.printf("Reading: Temp: ERR, Hum: ERR\n");
                    a->blink_to_show(MESSAGE_FAILED_TO_READ);    
                } else {
                    Serial.printf("Reading: Temp: %f, Hum: %f\n", temperature.value, humidity.value);
                    SignalAdd(temperatureAcc, temperature.value);
                    SignalAdd(humidityAcc, humidity.value);
                    if (CounterIncrease(smoother)) {
                        bt->setTemperature(SignalClose(temperatureAcc, smoother.targetCount));
                        bt->setHumidity(SignalClose(humidityAcc, smoother.targetCount));
                        bt->startAdvertising("SupernovaIoT");
                        performedRead = true;
                    } else {
                        a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR); // does not return
                    }
                }
            } else {
                if (bt->clientIsDone()) {
                    a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR); // does not return
                } else {
                    delay(100);
                    cyclesOnServer ++;
                    if (cyclesOnServer == 100) {
                        Serial.println("BLE: Client did not talked to us, continuing...");
                        a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR); // does not return
                    }
                }
            }
        }
        // ROLE SERVER
    #endif

    #if ROLE == ROLE_WIFI // ROLE_WIFI
        if (a->read_state() == APP_INIT) {
            a->handle_client();
            if (++cyclesOnServer == 50) {
                a->blink_to_show(MESSAGE_CONFIG_MODE_ENABLED);
                cyclesOnServer = 0;
            }
            delay(10);

        } else if (a->read_state() == APP_CONFIGURED) {
            DataReading temperature = a->read_temperature();
            DataReading humidity    = a->read_humidity();
            if (!temperature.success || !humidity.success) {
                Serial.printf("Reading: Temp: ERR, Hum: ERR\n");
                a->blink_to_show(MESSAGE_FAILED_TO_READ);
            } else {
                Serial.printf("Reading: Temp: %f, Hum: %f\n", temperature.value, humidity.value);
                SignalAdd(temperatureAcc, temperature.value);
                SignalAdd(humidityAcc, humidity.value);
                if (CounterIncrease(smoother)) {
                    a->start_wifi_client();
                    bool success = a->send_measurements_to_influx_server(
                        SignalClose(temperatureAcc, smoother.targetCount),
                        SignalClose(humidityAcc, smoother.targetCount)
                    );
                    if (!success) {
                        a->blink_to_show(MESSAGE_FAILED_TO_WRITE);
                    }
                }
            }
            a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR);

        } else if (a->read_state() == APP_WAKEUP) {

            if (a->isWakeUpButtonOn()) {
                if (++cyclesOnServer == 25) {
                    a->blink_to_show(MESSAGE_FAILED_TO_READ);
                    a->set_state(APP_INIT, true);
                } else {
                    Serial.print("*");
                    delay(100);
                }
            } else {
                a->set_state(APP_CONFIGURED, true);
            }
        }
    #endif // ROLE_WIFI

}
