#ifndef APPLICATION_H_
#define APPLICATION_H_

// application status codes
#define APP_INIT        0
#define APP_CONFIGURED  1

// application messages
#define MESSAGE_FAILED_TO_CONNECT   1
#define MESSAGE_FAILED_TO_READ      2
#define MESSAGE_FAILED_TO_WRITE     3

class IOAdapter {
public:
    virtual int read_state() = 0;
    virtual void set_state(int) = 0;
    virtual void start_AP_server() = 0;
    virtual void handle_client() = 0;
    virtual bool start_wifi_client() = 0;
    virtual void blink_to_show(int message) = 0;
    virtual void restart() = 0;

    virtual float read_temperature() = 0;
    virtual float read_humidity() = 0;
    virtual bool send_measurements_to_influx_server(float temperature, float humidity) = 0;
};

class Application {
    IOAdapter *adapter;

public:
    Application(IOAdapter *adapterInstance);
    void setup();
    void loop();
};

#endif /* APPLICATION_H_ */
