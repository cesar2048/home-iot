/**************************************************************************************

 ESP8266 NodeMCU interfacing with SSD1306 OLED and DHT22 (AM2302) sensor
 This is a free software with NO WARRANTY.
 http://simple-circuit.com/

***************************************************************************************/

#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>
#include <WiFi.h>

#define DHTPIN  15          // DHT22 data pin is connected to ESP8266 GPIO14 (NodeMCU D5)
#define DHTTYPE DHT22       // DHT22 sensor is used
DHT_Unified dht(DHTPIN, DHTTYPE);

char temperature[] = "000.00 C";
char humidity[]    = "000.00 %";

const char* ssid = "kamino";
const char* password = "";

const char* host = "192.168.2.233";
const int httpPort = 3000;



void setup(void)
{
  Serial.begin(115200);
  while(!Serial){delay(100);}

  // connect to wifi
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nWiFi connected: ");
  Serial.println(WiFi.localIP());

  
  dht.begin(); // Initialize the DHT library
  sensor_t sensor;

  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
}

void loop()
{
  // Get temperature event and print its value.
  sensors_event_t event1;
  dht.temperature().getEvent(&event1);
  if (isnan(event1.temperature)) {
    Serial.println(F("Error reading temperature!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(event1.temperature);
    Serial.println(F(" C"));
  }

  // Get humidity event and print its value.
  sensors_event_t event2;
  dht.humidity().getEvent(&event2);
  if (isnan(event2.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(event2.relative_humidity-2);
    Serial.println(F("%"));
  }

  WiFiClient client;
  if(!client.connect(host, httpPort)) {
    Serial.println("FATAL: Unable to connect");
    return;
  }

  String data = "{\"t\":"+String(event1.temperature)+",\"r\":"+String(event2.relative_humidity)+"}";
  postJSON(&client, "/data", &data);

  delay(1000*5);
}

void postJSON(WiFiClient *client, String url, String *data) {
  String headers = "POST "+url+" HTTP/1.1\r\nHost: " + String(host) + "\r\nContent-Length: "+data->length()+"\r\nContent-Type: application/json\r\nConnection:close\r\n\r\n";
  client->print(headers);
  client->print(*data);
}

