#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <PMserial.h>
// #include <U8g2lib.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "secrets.h"

#define SCL 4
#define SDA 5

#define I2C_SCL 14
#define I2C_SDA 12

// U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

SerialPM pms(PMS5003, Serial);

// TwoWire I2CBME = TwoWire(0);

// Adafruit_BMP280 bmp(&I2CBME);
Adafruit_BMP280 bmp;

WebServer server(80);

uint16_t ugA = 0;
uint16_t ugB = 0;
uint16_t ugC = 0;

float temperature, pressure, altitude;

unsigned long lastTime = 0;

String message = "AAA";

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
  // start server
  server.on("/", handleGet);
  server.onNotFound(handleNotFound);
  server.begin();
}

// void drawReadings() {
//   u8g2.setCursor(0, 16);
//   u8g2.print(WiFi.localIP());
//   u8g2.setCursor(0, 26);
//   u8g2.print(message);
// }

void setup() {
  Serial.begin(9600);
  // I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  Wire.begin(SDA, SCL, 115200);
  pms.init();

  if (bmp.begin(0x76)){
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500);
  } else {
    message = "BMP Not found";
  }

  // u8g2.begin();
  // u8g2.setFont(u8g2_font_6x10_tr);
  // u8g2.firstPage();

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
  
  // if (u8g2.nextPage()) {
  //   drawReadings();
  // }
}