#include "Application.hpp"

// RTC_DATA_ATTR int               failureWriteCount   = 0;
RTC_DATA_ATTR SignalAccumulator temperatureAcc = { 0, 0 };
RTC_DATA_ATTR SignalAccumulator humidityAcc    = { 0, 0 };
RTC_DATA_ATTR SmoothCounter     smoother       = { SMOOTHING_FACTOR, 0 };
RTC_DATA_ATTR int cycleCount = 0;
int cyclesOnServer           = 0;
bool performedRead           = false;

Application::Application(IOAdapter *adapterInstance, BTServer *btInstance, WiFiAdapter *wifiAdapter, SensorProvider *sensor):
    adapter(adapterInstance),
    bt(btInstance),
    wifi(wifiAdapter),
    sensor(sensor)
{
}

void Application::setup() {
    // Serial.println("SETUP: START");
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
        // Serial.println("SETUP: Sensor init");
        sensor->init();
    #endif

    #if ROLE == ROLE_WIFI || ROLE == BLE_CLIENT
        if (a->read_state() == APP_INIT) {
            Serial.println("Start in APP_INIT");
            wifi->start_AP_server();

        } else if (a->read_state() == APP_TEST) {
            Serial.println("Start as APP_TEST");

            bool wifiConnected = wifi->start_wifi_client();
            if (!wifiConnected) {
                Serial.println("failed to connect to wifi");
                a->blink_to_show(MESSAGE_FAILED_TO_CONNECT);
                a->set_state(APP_INIT, true);
            }

            a->set_state(APP_CONFIGURED, true);
        }

        if (a->read_state() == APP_CONFIGURED) {
            sensor->init();
            // Serial.println("SETUP: init finished");
        }
    #endif
    // Serial.println("SETUP: DONE");
}

void Application::loop() {
    // Serial.println("APP: loop()");
    IOAdapter *a = this->adapter;

    #if ROLE == ROLE_BLE_SERVER
        // ROLE SERVER
        // Serial.println("APP: BLE_SERVER app");
        if (a->read_state() == APP_INIT) {
            Serial.println("APP: mode APP_INIT, go show tp.blink");
            a->blink_to_show(MESSAGE_DEMO);
            delay(250);
        } else if (a->read_state() == APP_CONFIGURED) {
            if (!performedRead) {
                Serial.println("APP: mode APP_CONFIGURED");
                float temp, humi;
                a->blink_to_show(MESSAGE_READ);
                if (readValues(&temp, &humi)) {
                    performedRead = true;
                    bt->setvalues(temp, humi);
                    bt->startAdvertising("SupernovaIoT");
                    Serial.print("BLE: Waiting for client");
                    return;
                }
            } else {
                if (!bt->clientIsDone()) {
                    a->blink_to_show(MESSAGE_BLE_SERVER);
                    if (!cyclesDelay(25 /* max cycles */, 100 /* delay ms */)) {
                        Serial.print("*");
                        return;
                    }
                    Serial.println(".. Client did not talked to us, continue...");
                }
            }
            a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR);
        } else {
            Serial.printf("APP: mode unsupported %i\n", a->read_state());
        }
        // ROLE SERVER
    #endif

    #if ROLE == ROLE_WIFI || ROLE == ROLE_BLE_CLIENT // ROLE_WIFI
        if (a->read_state() == APP_INIT) {
            wifi->handle_client();
            if (cyclesDelay(50 /* max cycles */, 10 /* delay ms */)) {
                a->blink_to_show(MESSAGE_CONFIG_MODE_ENABLED);
            }

        } else if (a->read_state() == APP_CONFIGURED) {
            Serial.println("APP: status APP_CONFIGURED, read values");
            float temp, humi;
            if (readValues(&temp, &humi)) {
                wifi->start_wifi_client();
                bool success = wifi->send_measurements_to_influx_server(temp, humi);
                if (!success) {
                    a->blink_to_show(MESSAGE_FAILED_TO_WRITE);
                }
            }

            #if ROLE == ROLE_WIFI
                a->deepSleep((60 - 6) * 1000 / SMOOTHING_FACTOR);
            #else // ROLE_BLE_CLIENT
                Serial.println("APP: restart BLE");
                a->restart();
            #endif

        } else if (a->read_state() == APP_WAKEUP) {
            if (a->isWakeUpButtonOn()) {
                if (!cyclesDelay(25 /* max cycles */, 100 /* delay ms */)) {
                    Serial.print("*");
                } else {
                    a->blink_to_show(MESSAGE_FAILED_TO_READ);
                    a->set_state(APP_INIT, true);
                }
            } else {
                a->set_state(APP_CONFIGURED, true);
            }
        }
    #endif // ROLE_WIFI

}

/**
 * Returns true when the read correct and averaged
*/
bool Application::readValues(float *outTemp, float *outHumi) {
    Serial.printf("Reading from: ");

    IOAdapter *a = this->adapter;
    float temperature, humidity;
    if (!sensor->readValues(&temperature, &humidity)) {
        Serial.printf("Reading: Temp: ERR, Hum: ERR\n");
        a->blink_to_show(MESSAGE_FAILED_TO_READ);    
    } else {
        Serial.printf("Reading: Temp: %f, Hum: %f\n", temperature, humidity);
        SignalAdd(temperatureAcc, temperature);
        SignalAdd(humidityAcc, humidity);
        if (CounterIncrease(smoother)) {
            *outTemp = SignalClose(temperatureAcc, smoother.targetCount);
            *outHumi = SignalClose(humidityAcc, smoother.targetCount);
            return true;
        }
    }

    return false;
}

bool Application::cyclesDelay(int max, int delayMs) {
    delay(delayMs);
    cyclesOnServer ++;
    if (cyclesOnServer == max) {
        cyclesOnServer = 0;
        return true;
    }
    return false;
}