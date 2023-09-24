#include "EspAdapter.hpp"
#include "test-scripts/public/output.c"
#include "env.h"

const char *baseAPName = "Hyg";
#define LIMIT_EMPTY_REQUESTS 5

void printWakeUpReason(){
    esp_sleep_wakeup_cause_t wakeupReason;

    wakeupReason = esp_sleep_get_wakeup_cause();

    switch(wakeupReason){
      case ESP_SLEEP_WAKEUP_EXT0     : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
      case ESP_SLEEP_WAKEUP_EXT1     : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER    : Serial.println("Wakeup caused by timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
      case ESP_SLEEP_WAKEUP_ULP      : Serial.println("Wakeup caused by ULP program"); break;
      default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeupReason); break;
    }

    if (wakeupReason == ESP_SLEEP_WAKEUP_EXT0) {
        Preferences appPrefs;
        appPrefs.begin("appPrefs", PREFS_RW_MODE);
        appPrefs.putInt("state", APP_WAKEUP);
    }
}

ESP32Adapter::ESP32Adapter() : statusLedColor(0) {
}

void ESP32Adapter::init() {
    printWakeUpReason();

    #ifndef NEOPIXEL_POWER
        // Devkit Do-it
        pinMode(WAKEUP_PIN, INPUT);
        while(!Serial){delay(100);}
    #else
        pinMode(WAKEUP_PIN, INPUT); 
    #endif
}

int ESP32Adapter::read_state()
{
    int result = APP_INIT;
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RW_MODE);

    bool keyExists = appPrefs.isKey("state");
    if (keyExists) {
        result = appPrefs.getInt("state");
    } else {
        appPrefs.putInt("state", result);
    }

    return result;
}

void ESP32Adapter::set_state(int state, bool restart) {
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RW_MODE);
    appPrefs.putInt("state", state);

    if (restart) {
        this->restart();
    }
}



void ESP32Adapter::restart()
{
    ESP.restart();
}

void ESP32Adapter::init_sensors() {
    this->dht = new DHT_Unified(DHTPIN, DHTTYPE);
    this->dht->begin();
}

DataReading ESP32Adapter::read_temperature()
{
    sensors_event_t evt;
    this->dht->temperature().getEvent(&evt);

    if (isnan(evt.temperature)) {
        // Serial.println(F("Sensor: Temperature error"));
        return DataReading{false, 0 };
    }
    
    return DataReading{true, evt.temperature };
}

DataReading ESP32Adapter::read_humidity()
{
    sensors_event_t evt;
    this->dht->humidity().getEvent(&evt);

    if (isnan(evt.relative_humidity)) {
        // Serial.println(F("Sensor: Humidity error"));
        return DataReading{false, 0 };
    }

    return DataReading{true, evt.relative_humidity };
}


void ESP32Adapter::blink_to_show(int message)
{
    int count = 0;
    int speed = 500;
    int color = COLOR_GREEN; // green

    if (message == MESSAGE_CONFIG_MODE_ENABLED) {
        int status = this->statusLedColor;
        this->statusLed(!status ? COLOR_GREEN : 0);
        delay(25);
        this->statusLed(status ? COLOR_GREEN : 0);
        return;
    }

    switch(message) {
        case MESSAGE_FAILED_TO_CONNECT: color = COLOR_RED;  count = 2;              break;
        case MESSAGE_FAILED_TO_READ:    color = COLOR_RED;  count = 3; speed = 200; break;
        case MESSAGE_FAILED_TO_WRITE:   color = COLOR_RED;  count = 4; speed = 200; break;
    }
    
    while (count-- != 0) {
        this->statusLed(color);
        delay(speed);
        this->statusLed(COLOR_OFF);
        if (count != 0) {
            delay(speed);
        }
    }
}

void ESP32Adapter::statusLed(int color) {
    this->statusLedColor = color;
    #ifndef NEOPIXEL_POWER
        pinMode(INDICATOR_LED, OUTPUT);
        digitalWrite(INDICATOR_LED, color ? HIGH : LOW);
    #else
        #define NUMPIXELS 1
        Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_RGB + NEO_KHZ800);
        pinMode(NEOPIXEL_POWER, OUTPUT);
        if (color) {
            digitalWrite(NEOPIXEL_POWER, HIGH);
            pixels.begin();
            pixels.setBrightness(50);
            pixels.fill(color);
            pixels.show();
        } else {
            digitalWrite(NEOPIXEL_POWER, LOW);
        }
    #endif
}

void ESP32Adapter::deepSleep(int milliSeconds) {
    Serial.println("going deep sleep");
    esp_sleep_enable_timer_wakeup(1000 * milliSeconds);
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, WAKEUP_STATE);
    esp_deep_sleep_start();
}

int ESP32Adapter::isWakeUpButtonOn() {
    int pinStatus = digitalRead(WAKEUP_PIN);
    return pinStatus == WAKEUP_STATE;
}


// ----------------------------- WiFi section   ------------------------------------

#if ROLE == ROLE_WIFI

ESP32Wifi::ESP32Wifi() : server(80) {

}

void ESP32Wifi::start_AP_server()
{
    Serial.println(F("Configuring access point..."));

    IPAddress localIP(192, 168, 2, 1);
    IPAddress gateway(192, 168, 2, 1);
    IPAddress subnet(255, 255, 255, 0);

    String APName = String(baseAPName);
    String deviceId = WiFi.macAddress(); // ex: 30:AE:A4:07:0D:64
    deviceId.replace(":", "");           // ex: 30AEA4070D64
    APName += deviceId.substring(8, 12); // Hyg0D64

    WiFi.softAPConfig(localIP, gateway, subnet);
    if (!WiFi.softAP(APName.c_str()))
    {
        log_e("Soft AP creation failed.");
    }

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    
    this->server.begin();

    Serial.println("Server started");
}

void ESP32Wifi::handle_client()
{
    WiFiClient client = this->server.available(); // listen for incoming clients

    if (client) // we got a client
    {
        int emptyRequestCount = 0;
        Serial.println(F("Client connected"));
        while (client.connected())
        {
            String currentLine = "", method = "", url = "", body = "";

            bool hasAllHeaders = false;
            while (client.available()) // there are bytes available to read
            {
                char c = client.read();
                if (c != '\n') {
                    if (c != '\r') {
                        currentLine += c; // add it to the end of the currentLine
                    }
                } else {
                    if (method == "")
                    { // parse request line
                        size_t ini = 0, pos = currentLine.indexOf(" ");
                        method = currentLine.substring(0, pos);
                        method.toLowerCase();

                        ini = pos + 1;
                        pos = currentLine.indexOf(" ", ini);
                        url = currentLine.substring(ini, pos);
                    }
                    else if (!hasAllHeaders)
                    { // parse header line
                        if (currentLine == "") {
                            hasAllHeaders = true;
                        }
                    } else {
                        body += currentLine + "\n";
                    }

                    currentLine = "";
                }
            }

            // there may be no bytes available to read and parse just yet
            if (method == "") {
                emptyRequestCount ++;
                if (emptyRequestCount == LIMIT_EMPTY_REQUESTS) {
                    break;
                } else {
                    delay(10); // wait for possible network delays
                    continue;
                }
            }

            Serial.printf("Req: [%s] [%s]\n", method.c_str(), url.c_str());
            this->handle_request(client, method, url, body);
            break; // this simple server won't support keep-alive connections
        }

        // close the connection:
        client.stop();
        Serial.println("Client Disconnected");
    }
}

String parseValue(String& body, size_t& posStart) {
    Serial.printf("parseValue: posStart=%i, ", posStart);
    size_t posEQ = body.indexOf("=", posStart);
    size_t posLF = body.indexOf("\n", posEQ);
    posStart = posLF+1;

    Serial.printf("posEQ=%i, posLF=%i\n", posEQ, posLF);
    return body.substring(posEQ+1, posLF);
}

void ESP32Wifi::handle_request(WiFiClient &client, String &method, String &url, String &body) {
    // Write response code and HTTP headers
    if (method == "get" && url == "/") {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.printf("Content-Length:%i\n", index_html_length);
        client.println("Content-Encoding:gzip");
        client.println("Connection:close");
        client.println();
        client.write(index_html, index_html_length);

    } else if (method == "post" && url == "/set-wifi") {
        // body="ssid=ABCD\npassword=DEFG\ninfluxUrl=http://192.168.1.1"
        //       ^          ^         ^          ^         ^
        //       0         10        20         30        40
        Serial.println("body: " + body);
        size_t parserPos = 0;
        String ssid         = parseValue(body, parserPos); // posStart=0, posEQ=4, posLF=9
        String pass         = parseValue(body, parserPos); // posStart=10, posEQ=18, posLF=23
        String deviceName   = parseValue(body, parserPos);
        String influxUrl    = parseValue(body, parserPos);
        String influxToken  = parseValue(body, parserPos);
        String influxOrg    = parseValue(body, parserPos);
        String influxBucket = parseValue(body, parserPos);
        
        Serial.printf("Params check:\nSSID=[%s]\nPASS=[%s]\nDeviceName=[%s]\nInfluxURL=[%s]\nInfluxToken=[%s]\nInfluxOrg=[%s]\nInfluxBucket=[%s]\n",
          ssid.c_str(), pass.c_str(), deviceName.c_str(), influxUrl.c_str(), influxToken.c_str(), influxOrg.c_str(), influxBucket.c_str());

        Preferences appPrefs;
        appPrefs.begin("appPrefs", PREFS_RW_MODE);
        appPrefs.putInt("state", APP_TEST);
        appPrefs.putString("ssid", ssid);
        appPrefs.putString("pass", pass);
        appPrefs.putString("deviceName", deviceName);
        appPrefs.putString("influxUrl", influxUrl);
        appPrefs.putString("influxToken", influxToken);
        appPrefs.putString("influxOrg", influxOrg);
        appPrefs.putString("influxBucket", influxBucket);

        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

        // this->restart();
        // TODO: Propagate reference to restart in APP level

    } else if (url == "/H") {
        this->statusLed(COLOR_GREEN);
        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

    } else if (url == "/L") {
        this->statusLed(COLOR_OFF);
        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

    } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Connection:close");
        client.println();
    }
}

bool ESP32Wifi::start_wifi_client()
{
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RO_MODE);
    String ssid = appPrefs.getString("ssid");
    String pass = appPrefs.getString("pass");
    
    // Setup wifi
    WiFi.mode(WIFI_STA);
    this->wifiMulti = new WiFiMulti();
    this->wifiMulti->addAP(ssid.c_str(), pass.c_str());

    int tries = 10;
    Serial.print(F("Connecting to wifi"));
    while (this->wifiMulti->run() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
        tries --;
        if (tries == 0) {
            Serial.println(F("Failed to connect to wifi"));
            return false;
        }
    }
    Serial.println();

    // Accurate time is necessary for certificate validation and writing in batches
    // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial.
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println(F("Time not configured, sync"));
        timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    }


    String influxUrl    = appPrefs.getString("influxUrl");
    String influxToken  = appPrefs.getString("influxToken");
    String influxOrg    = appPrefs.getString("influxOrg");
    String influxBucket = appPrefs.getString("influxBucket");
    InfluxDBClient client(influxUrl, influxOrg, influxBucket, influxToken);

    if (client.validateConnection()) {
        Serial.print(F("InfluxDB: connected "));
        Serial.println(client.getServerUrl());
    } else {
        Serial.print(F("InfluxDB: connection failed: "));
        Serial.println(client.getLastErrorMessage());
        return false;
    }

    return true;
}


bool ESP32Wifi::send_measurements_to_influx_server(float temperature, float humidity)
{
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RO_MODE);
    String influxUrl    = appPrefs.getString("influxUrl");
    String influxToken  = appPrefs.getString("influxToken");
    String influxOrg    = appPrefs.getString("influxOrg");
    String influxBucket = appPrefs.getString("influxBucket");
    String deviceName   = appPrefs.getString("deviceName");
    
    String deviceId = WiFi.macAddress(); // example: 30:AE:A4:07:0D:64
    deviceId.replace(":", "");           // example: 30AEA4070D64

    InfluxDBClient client(influxUrl, influxOrg, influxBucket, influxToken);
    Point influxSensor("ambient_status");
    influxSensor.clearFields();
    influxSensor.addTag("hwid", "Hygro" + deviceId.substring(6, 12)); // Hygro070D64
    influxSensor.addTag("device", deviceName);
    influxSensor.addField("temperature", temperature);
    influxSensor.addField("humidity", humidity);

    Serial.print(F(" - Writing: "));
    Serial.println(influxSensor.toLineProtocol());

    // Check WiFi connection and reconnect if needed
    if (this->wifiMulti->run() != WL_CONNECTED) {
        Serial.println(F("Wifi connection lost"));
    }

    // Write point
    if (!client.writePoint(influxSensor)) {
        Serial.print(F("InfluxDB: write failed, "));
        Serial.println(client.getLastErrorMessage());
        return false;
    }

    return true;
}
#endif

// ----------------------------- WiFi section   ------------------------------------

#if ROLE == ROLE_WIFI
#else

void DummyWifi::start_AP_server() {
    Serial.println("DummyWifi: start_AP_server()");
}
void DummyWifi::handle_client() {
    Serial.println("DummyWifi: handle_client()");
}
bool DummyWifi::start_wifi_client() {
    Serial.println("DummyWifi: start_wifi_client()");
}
bool DummyWifi::send_measurements_to_influx_server(float temperature, float humidity) {
    Serial.println("DummyWifi: send_measurements_to_influx_server");
}

#endif

// ----------------------------- bluetooth section ----------------------------------

CharacteristicCallbacks::CharacteristicCallbacks(ESPBTAdapter *adapter): btAdapter(adapter) {
}

void CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param) {
    Serial.printf("BLE: Client wrote: %s\n", pCharacteristic->getValue().c_str());
    btAdapter->clientWroteSomething = true;
}




class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("BLE: Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("BLE: Client disconnected");
    }
};




ESPBTAdapter::ESPBTAdapter(): temp(0), humidity(0), clientWroteSomething(false) {
}

void ESPBTAdapter::startAdvertising(std::string deviceName) {
    // Create the BLE Device
    BLEDevice::init("ESP32");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
        );

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
    float values[2] = {temp, humidity};
    pCharacteristic->setValue((uint8_t*)values, 8);
    Serial.printf("BLE: values available {%f, %f}\n", temp, humidity);

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("BLE: Advertising, waiting for client connection");
}

void ESPBTAdapter::setTemperature(float value) {
    temp = value;
}

void ESPBTAdapter::setHumidity(float value) {
    humidity = value;
}

bool ESPBTAdapter::clientIsDone() {
    return clientWroteSomething;
}
