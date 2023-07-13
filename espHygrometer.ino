/**************************************************************************************
 ESP8266 NodeMCU interfacing with SSD1306 OLED and DHT22 (AM2302) sensor
***************************************************************************************/
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Wire.h>
#include <DHT.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "SmoothSignal.hpp"
#include "env.h"

#define DHTPIN  15
#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);
WiFiMulti wifiMulti;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool influxValidated = false;

// Declare InfluxDB client and data point
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point influxSensor("ambient_status");
RTC_DATA_ATTR SignalAccumulator temperatureSignal = { 0, 0 };
RTC_DATA_ATTR SignalAccumulator humiditySignal  = { 0, 0 };
RTC_DATA_ATTR SmoothCounter smoothCount = { SMOOTHING_FACTOR, 0 };

void setup(void)
{
    Serial.begin(115200);
    while(!Serial){delay(100);}

    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.print(F("-------- Boot number: ")); Serial.print(bootCount);

    if (bootCount == 1) {
        uint32_t Freq = 0;
        Freq = getCpuFrequencyMhz();
        Serial.print(F(" CPU Freq = ")); Serial.print(Freq); Serial.println(F(" MHz"));
        Freq = getXtalFrequencyMhz();
        Serial.print(F("XTAL Freq = ")); Serial.print(Freq); Serial.println(F(" MHz"));
        Freq = getApbFrequency();
        Serial.print(F("APB Freq = ")); Serial.print(Freq); Serial.println(F(" Hz"));
    } else {
      //Print the wakeup reason for ESP32
      print_wakeup_reason();
    }
    
    dht.begin(); // Initialize the DHT library
}

void initConnection() {
// Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);

  Serial.print(F("Connecting to wifi"));
  while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
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

  // Check server connection
  if (!influxValidated) {
    if (client.validateConnection()) {
        Serial.print(F("InfluxDB: connected"));
        Serial.println(client.getServerUrl());
    } else {
        Serial.print(F("InfluxDB: connection failed: "));
        Serial.println(client.getLastErrorMessage());
    }
    influxValidated = true;
  }

  influxSensor.addTag("device", "ESP32");
}


void loop()
{
  // Get temperature event and print its value.
  sensors_event_t evtTemperature, evtHumidity;
  dht.temperature().getEvent(&evtTemperature);
  dht.humidity().getEvent(&evtHumidity);

  if (isnan(evtTemperature.temperature)) {
    Serial.println(F("Sensor: Temperature error"));
  } else if (isnan(evtHumidity.relative_humidity)) {
    Serial.println(F("Sensor: Humidity error"));
  }

  Serial.print(F("Sensor: Temperature: ")); Serial.print(evtTemperature.temperature);
  Serial.print(F(" C, Humidity: ")); Serial.print(evtHumidity.relative_humidity); Serial.print(F(" %"));

  SignalAdd(temperatureSignal, evtTemperature.temperature);
  SignalAdd(humiditySignal, evtHumidity.relative_humidity);
  if (CounterIncrease(smoothCount)) {
    initConnection();

    influxSensor.clearFields();
    influxSensor.addField("temperature", SignalClose(temperatureSignal, smoothCount.targetCount));
    influxSensor.addField("humidity", SignalClose(humiditySignal, smoothCount.targetCount));
    
    Serial.print(F(" - Writing: "));
    Serial.println(influxSensor.toLineProtocol());

    // Check WiFi connection and reconnect if needed
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println(F("Wifi connection lost"));
    }
  
    // Write point
    if (!client.writePoint(influxSensor)) {
      Serial.print(F("InfluxDB: write failed, "));
      Serial.println(client.getLastErrorMessage());
    }
  }

  esp_sleep_enable_timer_wakeup(1000 * 1000 * (60 - 6) / SMOOTHING_FACTOR);
  esp_deep_sleep_start();
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :     Serial.println(F(" by ext signal RTC_IO")); break;
    case ESP_SLEEP_WAKEUP_EXT1 :     Serial.println(F(" by ext signal RTC_CNTL")); break;
    case ESP_SLEEP_WAKEUP_TIMER :    Serial.println(F(" by timer")); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println(F(" by touchpad")); break;
    case ESP_SLEEP_WAKEUP_ULP :      Serial.println(F(" by ULP")); break;
    default :                        Serial.printf (" not by deep sleep: %d\n",wakeup_reason); break;
  }
}

