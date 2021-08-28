#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT22  // change to DHT11 if that's what you have

#define WLAN_SSID "YOUR_WIFI_SSID"
#define WLAN_PASS "YOUR_WIFI_PASSWORD"

ESP8266WebServer server(80);

DHT dht(DHTPIN, DHTTYPE, 15);

float humidity, temp_c, temp_f, heatindex;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connecting to wifi
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  // Set a static IP (optional)
  IPAddress ip(192, 168, 1, 55);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to wifi with IP ");
  Serial.println(WiFi.localIP());

  // Define capabilities of our little web server
  server.on("/", handle_root);

  server.on("/temp", []() {
    ReadSensor();
    server.send(200, "text/plain", String(temp_f));
  });

  server.on("/temp_c", []() {
    ReadSensor();
    server.send(200, "text/plain", String(temp_c));
  });

  server.on("/humidity", []() {
    ReadSensor();
    server.send(200, "text/plain", String(humidity));
  });

  server.on("/heatindex", []() {
    ReadSensor();
    server.send(200, "text/plain", String(heatindex));
  });

  server.begin();
  Serial.println("HTTP server started");

  dht.begin();
}

void handle_root() {
  server.send(200, "text/plain", "All systems go. Read data from /temp or or /temp_c or /humidity or /heatindex.");
  delay(100);
}

void loop() {
  server.handleClient();
}

void ReadSensor() {
  // Read humidity (percent)
  humidity = dht.readHumidity();
  // Read temperature as Celsius
  temp_c = dht.readTemperature();
  // Read temperature as Fahrenheit
  temp_f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temp_c) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor :-(");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  heatindex = dht.computeHeatIndex(temp_f, humidity);
}
