#include "ESP32IO.hpp"

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
        Serial.print(F("IO::prefs[state] exists = "));
        Serial.print(result);
        Serial.print(F("\n"));
    }
    else
    {
        Serial.print(F("IO::prefs[state] undefined\n"));
        appPrefs.putInt("state", result);
    }

    return result;
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

    if (client)
    {                                  // if you get a client,
        Serial.println("New Client."); // print a message out the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                if (c == '\n')
                { // if the byte is a newline character

                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // the content of the HTTP response follows the header:
                        client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
                        client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");

                        // The HTTP response ends with another blank line:
                        client.println();
                        // break out of the while loop:
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine:
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }

                // Check to see if the client request was "GET /H" or "GET /L":
                if (currentLine.endsWith("GET /H"))
                {
                    digitalWrite(LED_BUILTIN, HIGH); // GET /H turns the LED on
                }
                if (currentLine.endsWith("GET /L"))
                {
                    digitalWrite(LED_BUILTIN, LOW); // GET /L turns the LED off
                }
            }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

bool ESP32Adapter::start_wifi_client()
{
    return false;
}
void ESP32Adapter::blink_to_show(int message)
{
}
void ESP32Adapter::restart()
{
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
