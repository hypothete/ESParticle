#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <PMserial.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <HttpClient.h>
#include "secrets.h"

#define SCL 22
#define SDA 21

SerialPM pms(PMS5003, Serial2);
Adafruit_BMP280 bmp;
WebServer server(80);
HTTPClient http;

uint16_t pm1 = 0;
uint16_t pm25 = 0;
uint16_t pm10 = 0;

float temperature, pressure, altitude;

unsigned long lastTime = 0;

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(1000);
    ESP.restart();
  }
}

void updateMicrograms() {
  pms.read();
  // Serial.printf("PM1.0 %2d, PM2.5 %2d, PM10 %2d [ug/m3]\n",
  //                 pms.pm01, pms.pm25, pms.pm10);
  pm1 = pms.pm01;
  pm25 = pms.pm25;
  pm10 = pms.pm10;
}

void updatePAT() {
  temperature = bmp.readTemperature();
  pressure =    bmp.readPressure() / 100.0;
  altitude =    bmp.readAltitude(1014.75);
}

void handleGet() {
  // server.sendHeader("Access-Control-Allow-Origin", "*");
  String values = "";
  values += pm1;
  values += "\n";
  values += pm25;
  values += "\n";
  values += pm10;
  values += "\n";
  values += pressure;
  values += "\n";
  values += altitude;
  values += "\n";
  values += temperature;
  values += "\n";
  server.send(200, "text/plain", values);
}

void handleNotFound() {
  server.send(404, "text/plain", "File not found\n");
}

void postToThingsboard() {
  char json[200];
  sprintf(
    json,
    "{ \"pm1\": %u, \"pm25\": %u, \"pm10\": %u, \"altitude\": %f, \"pressure\": %f, \"temperature\": %f }",
    pm1, pm25, pm10, altitude, pressure, temperature
  );
  http.begin(THINGSBOARD_URL);
  http.addHeader("Content-Type", "application/json");
  http.POST(json); // returns int error
  http.end();
}

void setupServer() {
  server.on("/", handleGet);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Wire.begin(SDA, SCL, 115200);
  pms.init();

  if (bmp.begin(0x76)){
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500);
  }

  connectToWifi();
  setupServer();
}

void loop() {
  unsigned long timeNow = millis();
  if (timeNow - lastTime > 30000) {
    updateMicrograms();
    updatePAT();
    postToThingsboard();
    lastTime = timeNow;
  }

  server.handleClient();
}