#include "Application.hpp"

RTC_DATA_ATTR int               failureWriteCount   = 0;
RTC_DATA_ATTR SignalAccumulator temperatureAcc = { 0, 0 };
RTC_DATA_ATTR SignalAccumulator humidityAcc    = { 0, 0 };
RTC_DATA_ATTR SmoothCounter     smoother       = { SMOOTHING_FACTOR, 0 };
int cyclesOnServer = 0;

Application::Application(IOAdapter *adapterInstance):adapter(adapterInstance) {
}

void Application::setup() {
    pinMode(INDICATOR_LED, OUTPUT);

    IOAdapter *a = this->adapter;
    if (a->read_state() == APP_INIT) {
        Serial.println("Start in APP_INIT");
        a->start_AP_server();

    } else if (a->read_state() == APP_TEST) {
        Serial.println("Start as APP_TEST");

        bool wifiConnected = a->start_wifi_client();
        if (!wifiConnected) {
            Serial.println("failed to connect to wifi");
            a->blink_to_show(MESSAGE_FAILED_TO_CONNECT);
            a->set_state(APP_INIT);
            a->restart();
        }

        a->set_state(APP_CONFIGURED);
        a->restart();
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
            delay(1000);
            return;
        }
        
        Serial.printf("Reading: Temp: %f, Hum: %f\n", temperature.value, humidity.value);
        SignalAdd(temperatureAcc, temperature.value);
        SignalAdd(humidityAcc, humidity.value);
        if (CounterIncrease(smoother)) {
            a->start_wifi_client();
            bool send_success = a->send_measurements_to_influx_server(
                SignalClose(temperatureAcc, smoother.targetCount),
                SignalClose(humidityAcc, smoother.targetCount)
            );
            if (!send_success) {
                a->blink_to_show(MESSAGE_FAILED_TO_WRITE);
                if (++failureWriteCount == MAX_WRITE_FAILURES) {
                    Serial.println("Too many failures, reset");
                    a->set_state(APP_INIT);
                    a->restart();
                }
            }
        }

        Serial.println("going deep sleep");
        esp_sleep_enable_timer_wakeup(1000 * 1000 * (60 - 6) / SMOOTHING_FACTOR);
        esp_deep_sleep_start();
    }
}
