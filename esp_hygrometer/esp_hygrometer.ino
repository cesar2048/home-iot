/**************************************************************************************

 ESP8266 NodeMCU interfacing with SSD1306 OLED and DHT22 (AM2302) sensor
 This is a free software with NO WARRANTY.
 http://simple-circuit.com/

***************************************************************************************/

#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>
// #include <WiFi.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DHTPIN  15          // DHT22 data pin is connected to ESP8266 GPIO14 (NodeMCU D5)
#define DHTTYPE DHT22       // DHT22 sensor is used
DHT_Unified dht(DHTPIN, DHTTYPE);
WiFiMulti wifiMulti;

#define INFLUXDB_URL "http://192.168.5.10:8086"
#define INFLUXDB_TOKEN ""
#define INFLUXDB_ORG "1555812480ef0cff"
#define INFLUXDB_BUCKET "HomeIoT"
// Time zone info
#define TZ_INFO "UTC-6"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
// Declare Data point
Point influxSensor("wifi_status");

// sensor readings
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
  /*WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nWiFi connected: ");
  Serial.println(WiFi.localIP());*/

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
  }
  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
  } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
  }

  influxSensor.addTag("device", "ESP32");
  influxSensor.addTag("SSID", WiFi.SSID());

  
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
  Serial.println(F("-----"));
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

  /*
  WiFiClient client;
  if(!client.connect(host, httpPort)) {
    Serial.println("FATAL: Unable to connect");
    return;
  }

  String data = "{\"t\":"+String(event1.temperature)+",\"r\":"+String(event2.relative_humidity)+"}";
  postJSON(&client, "/data", &data);
  */

  influxSensor.clearFields();
  influxSensor.addField("rssi", WiFi.RSSI());
  Serial.print("Writing: ");
  Serial.println(influxSensor.toLineProtocol());

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(influxSensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  delay(1000*60);
}

void postJSON(WiFiClient *client, String url, String *data) {
  String headers = "POST "+url+" HTTP/1.1\r\nHost: " + String(host) + "\r\nContent-Length: "+data->length()+"\r\nContent-Type: application/json\r\nConnection:close\r\n\r\n";
  client->print(headers);
  client->print(*data);
}

