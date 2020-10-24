#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <PMserial.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "secrets.h"

#define SCL 4
#define SDA 5

SerialPM pms(PMS5003, Serial);

Adafruit_BMP280 bmp;

WebServer server(80);

uint16_t ugA = 0;
uint16_t ugB = 0;
uint16_t ugC = 0;

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
  ugA = pms.pm01;
  ugB = pms.pm25;
  ugC = pms.pm10;
}

void updatePAT() {
  temperature = bmp.readTemperature();
  pressure =    bmp.readPressure();
  altitude =    bmp.readAltitude();
}

void handleGet() {
  // server.sendHeader("Access-Control-Allow-Origin", "*");
  String values = "";
  values += ugA;
  values += "\n";
  values += ugB;
  values += "\n";
  values += ugC;
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
  if (timeNow - lastTime > 3000) {
    updateMicrograms();
    updatePAT();
    lastTime = timeNow;
  }

  server.handleClient();
}