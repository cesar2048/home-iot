#include "EspAdapter.hpp"
#include "test-scripts/public/output.c"
#include "env.h"

const char *ssid = "SupernovaIoT";
#define LIMIT_EMPTY_REQUESTS 5

ESP32Adapter::ESP32Adapter() : server(80)
{
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

void ESP32Adapter::set_state(int state) {
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RW_MODE);
    appPrefs.putInt("state", state);
}

void ESP32Adapter::start_AP_server()
{
    Serial.println(F("Configuring access point..."));

    IPAddress localIP(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(localIP, gateway, subnet);
    if (!WiFi.softAP(ssid))
    {
        log_e("Soft AP creation failed.");
    }

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    this->server.begin();

    Serial.println("Server started");
}

void ESP32Adapter::handle_client()
{
    WiFiClient client = this->server.available(); // listen for incoming clients

    if (client) // we got a client
    {
        int emptyRequestCount = 0;
        Serial.println(F("New Client"));
        while (client.connected())
        {
            String currentLine = "", method = "", url = "", body = "";

            bool hasAllHeaders = false;
            while (client.available()) // there are bytes available to read
            {
                char c = client.read();
                if (c != '\n') {
                    if (c != '\r') {
                        // Serial.write(c);
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
                        //Serial.println("Method: " + method);
                        //Serial.println("Url   : " + url);
                    }
                    else if (!hasAllHeaders)
                    { // parse header line
                        if (currentLine != "") {
                            // Serial.print("Header =>"); Serial.println(currentLine.c_str());
                        } else {
                            hasAllHeaders = true;
                        }
                    } else {
                        // Serial.print("-- body => "); Serial.println(currentLine.c_str());
                        body += currentLine + "\n";
                    }

                    currentLine = "";
                }
            }

            // there may be no bytes available to read and parse just yet
            if (method == "") {
                emptyRequestCount ++;
                if (emptyRequestCount == LIMIT_EMPTY_REQUESTS) {
                    Serial.println("B");
                    break;
                } else {
                    Serial.print("W");
                    delay(10);
                    continue;
                }
            }

            Serial.printf("-- Process: method(%s), url(%s)\n", method.c_str(), url.c_str());

            this->handle_request(client, method, url, body);
            break; // this simple server won't support keep-alive connections
        }

        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

String parseValue(String& body, size_t& posStart) {
    Serial.printf("parseValue: posStart=%i, ", posStart);
    size_t posEQ = body.indexOf("=", posStart);
    size_t posLF = body.indexOf("\n", posEQ);
    posStart = posLF+1;

    Serial.printf("posEQ=%i, posLF=%i\n", posStart, posEQ, posLF);
    return body.substring(posEQ+1, posLF);
}

void ESP32Adapter::handle_request(WiFiClient &client, String &method, String &url, String &body) {
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
        String influxUrl    = parseValue(body, parserPos); // posStart=23
        String influxToken  = parseValue(body, parserPos);
        String influxOrg    = parseValue(body, parserPos);
        String influxBucket = parseValue(body, parserPos);
        
        Serial.printf("SSID=%s\nPASS=%s\nInfluxURL=%s\nInfluxToken=%s\nInfluxOrg=%s\nInfluxBucket=%s\n", ssid.c_str(), pass.c_str(), influxUrl.c_str(), influxToken.c_str(), influxOrg.c_str(), influxBucket.c_str());

        Preferences appPrefs;
        appPrefs.begin("appPrefs", PREFS_RW_MODE);
        appPrefs.putInt("state", APP_TEST);
        appPrefs.putString("ssid", ssid);
        appPrefs.putString("pass", pass);
        appPrefs.putString("influxUrl", influxUrl);
        appPrefs.putString("influxToken", influxToken);
        appPrefs.putString("influxOrg", influxOrg);
        appPrefs.putString("influxBucket", influxBucket);

        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

        this->restart();

    } else if (url == "/H") {
        digitalWrite(LED_BUILTIN, HIGH); // GET /H turns the LED on
        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

    } else if (url == "/L") {
        digitalWrite(LED_BUILTIN, LOW); // GET /L turns the LED off
        client.println("HTTP/1.1 302 Redirect");
        client.println("Location:/");
        client.println();

    } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Connection:close");
        client.println();
    }
}

bool ESP32Adapter::start_wifi_client()
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

    return true;
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

bool ESP32Adapter::send_measurements_to_influx_server(float temperature, float humidity)
{
    Preferences appPrefs;
    appPrefs.begin("appPrefs", PREFS_RO_MODE);
    String influxUrl    = appPrefs.getString("influxUrl");
    String influxToken  = appPrefs.getString("influxToken");
    String influxOrg    = appPrefs.getString("influxOrg");
    String influxBucket = appPrefs.getString("influxBucket");

    String deviceId = WiFi.macAddress(); // example: 30:AE:A4:07:0D:64
    deviceId.replace(":", "");           // example: 30AEA4070D64

    InfluxDBClient client(influxUrl, influxOrg, influxBucket, influxToken);
    Point influxSensor("ambient_status");
    influxSensor.clearFields();
    influxSensor.addField("temperature", temperature);
    influxSensor.addField("humidity", humidity);
    influxSensor.addTag("device", "Hygro" + deviceId.substring(6, 12)); // Hygro070D64

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


void ESP32Adapter::blink_to_show(int message)
{
    int count = 0;
    int speed = 500;

    switch(message) {
        case MESSAGE_FAILED_TO_CONNECT: count = 2;              break;
        case MESSAGE_FAILED_TO_READ:    count = 3; speed = 200; break;
        case MESSAGE_FAILED_TO_WRITE:   count = 4; speed = 200; break;
    }
    
    while (count != 0) {
        count --;
        digitalWrite(LED_BUILTIN, HIGH);
        delay(speed);
        digitalWrite(LED_BUILTIN, LOW);
        if (count != 0) {
            delay(speed);
        }
    }
}