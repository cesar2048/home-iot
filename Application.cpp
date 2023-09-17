#include "Application.hpp"

// RTC_DATA_ATTR int               failureWriteCount   = 0;
RTC_DATA_ATTR SignalAccumulator temperatureAcc = { 0, 0 };
RTC_DATA_ATTR SignalAccumulator humidityAcc    = { 0, 0 };
RTC_DATA_ATTR SmoothCounter     smoother       = { SMOOTHING_FACTOR, 0 };
int cyclesOnServer = 0;
RTC_DATA_ATTR int cycleCount = 0;

Application::Application(IOAdapter *adapterInstance):adapter(adapterInstance) {
}

void Application::setup() {
    IOAdapter *a = this->adapter;
    a->init();

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
}

void Application::loop() {
    IOAdapter *a = this->adapter;

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
    
    /*
    int val = digitalRead(GPIO_NUM_16);
    if (val) {
        Serial.print("*");
    } else {
        Serial.print("_");
    }
    if (cycleCount++ == 0) {
        a->deepSleep(10*1000); // 10s
    } else {
        delay(100);
    }*/
}
