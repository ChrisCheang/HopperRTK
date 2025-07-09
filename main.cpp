#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "base64.h"

const char* WIFI_ssid     = "LAPTOP-RGQJDUFO 3414";
const char* WIFI_password = "76$7u9J7";
const char* casterHost = "3.143.243.81";
const int   casterPort = 2101;
const char* mountpoint = "SkywalkerRTKBase";
const char* ntripUser  = "chrischeanghk@gmail.com";
const char* ntripPass  = "Skywalker";

Adafruit_NeoPixel onboardLED(1, 48, NEO_GRB + NEO_KHZ800);
WiFiClient client;
HardwareSerial GNSSserial(1); // UART1

void connectNTRIP() {
  Serial.println("Connecting to NTRIP caster...");

  if (client.connect(casterHost, casterPort)) {

    String auth = base64::encode(String(ntripUser) + ":" + String(ntripPass));
    String req  = "GET /" + String(mountpoint) + " HTTP/1.0\r\n";

    req += "User-Agent: NTRIP ESP32 1.0\r\n";
    req += "Authorization: Basic " + String(auth) + "\r\n\r\n";
    client.write((const uint8_t*)req.c_str(), req.length());
    client.flush();   // make sure it leaves the buffer

    Serial.println("Sent header:\n" + req);

    while (client.connected() && !client.available()) {
      onboardLED.setPixelColor(0, onboardLED.Color(255, 255, 0)); // Yellow LED indicates waiting
      onboardLED.show();
      delay(100);
    }

    // Read the response header

    String header = "";
    while (client.available()) {
      char c = client.read();
      header += c;
      if (header.endsWith("\r\n\r\n")) break;
    }

    Serial.println("Caster Response Header:");
    Serial.println(header);

    if (!header.startsWith("ICY 200 OK") && header.indexOf("200 OK") == -1) {
      Serial.println("NTRIP Caster did not accept connection.");
      client.stop();
      onboardLED.setPixelColor(0, onboardLED.Color(255, 0, 0)); // Red LED indicates failure
      onboardLED.show();
      return;
    }

    Serial.println("Receiving RTCM/NMEA data...");
    onboardLED.setPixelColor(0, onboardLED.Color(0, 255, 0));
    onboardLED.show();


  } else {
    Serial.println("Failed to connect to NTRIP caster.");
    onboardLED.setPixelColor(0, onboardLED.Color(255, 0, 0)); // Red LED indicates failure
    onboardLED.show();
    return;
  }
}

void setup() {
  Serial.begin(115200);

  onboardLED.begin();
  onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 0));
  onboardLED.setBrightness(10);
  onboardLED.show();

  GNSSserial.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_ssid, WIFI_password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi connected");
  // Blue LED indicates WiFi is connected
  onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255));\
  onboardLED.show();

  connectNTRIP();

}

void loop() {
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    // No LED indicates WiFi is disconnected
    onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 0));
    onboardLED.show();
    WiFi.reconnect();

    delay(1000); // Wait for a second

    if (WiFi.status() == WL_CONNECTED) {
      onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255)); // Blue LED indicates WiFi is connected
      onboardLED.show();
    }

  }

  if (client.connected()) {
    while (client.available()) {
      uint8_t byte = client.read();

        // Print each byte as two-digit hex
      if (byte < 0x10) Serial.print("0");
      Serial.print(byte, HEX);
      Serial.print(" ");

      GNSSserial.write(byte); // Send RTCM byte to GNSS serial port
    }
  } else {
    connectNTRIP(); // Reconnect if dropped
  }
}
