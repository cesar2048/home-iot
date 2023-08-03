#include "EspAdapter.hpp"
#include "test-scripts/public/output.c"

const char *ssid = "SupernovaIoT";

ESP32Adapter::ESP32Adapter() : server(80)
{
}

int ESP32Adapter::read_state()
{
    int result = APP_INIT;
    Preferences appPrefs;
    appPrefs.begin("appPrefs", RW_MODE);

    bool keyExists = appPrefs.isKey("state");
    if (keyExists)
    {
        result = appPrefs.getInt("state");
        // Serial.print(F("IO::prefs[state] exists = "));
        // Serial.print(result);
        // Serial.print(F("\n"));
    }
    else
    {
        // Serial.print(F("IO::prefs[state] undefined\n"));
        appPrefs.putInt("state", result);
        appPrefs.putString("ssid", "");
        appPrefs.putString("pass", "");
    }

    return result;
}

void ESP32Adapter::set_state(int state) {
    Preferences appPrefs;
    appPrefs.begin("appPrefs", RW_MODE);
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

#define LIMIT_EMPTY_REQUESTS 5

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
        // body="ssid=ABCD\npassword=DEFG\n"
        size_t posEQ = body.indexOf("=");
        size_t posLF = body.indexOf("\n");
        String ssid = body.substring(posEQ+1, posLF);
        Serial.println("SSID = " + ssid);

        posEQ = body.indexOf("=", posLF);
        String pass = body.substring(posEQ+1, body.length()-1);
        Serial.println("PASS = " + pass);

        Preferences appPrefs;
        appPrefs.begin("appPrefs", RW_MODE);
        appPrefs.putInt("state", APP_CONFIGURED);
        appPrefs.putString("ssid", ssid);
        appPrefs.putString("pass", pass);

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
    appPrefs.begin("appPrefs", RO_MODE);
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
void ESP32Adapter::blink_to_show(int message)
{
    int count = 0;
    int speed = 300;
    switch(message) {
        case MESSAGE_FAILED_TO_CONNECT: count = 2; break;
        case MESSAGE_FAILED_TO_READ:    count = 3; break;
        case MESSAGE_FAILED_TO_WRITE:   count = 4; break;
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
void ESP32Adapter::restart()
{
    ESP.restart();
}


float ESP32Adapter::read_temperature()
{
    return 0;
}
float ESP32Adapter::read_humidity()
{
    return 0;
}
bool ESP32Adapter::send_measurements_to_influx_server(float temperature, float humidity)
{
    return false;
}
