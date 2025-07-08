#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "mbedtls/base64.h"

// Wi-Fi credentials
const char *ssid = "Xiaomi 11T Pro";
const char *password = "Ohyeaheh";

// NTRIP Caster credentials
const char *ntripHost = "RTK2go.com";  // Domain is better than IP for redirects
const int ntripPort = 2101;
const char *mountpoint = "SkywalkerRTKBase";
const char *ntripUser = "chrischeanghk@gmail.com";
const char *ntripPass = "Skywalker";

WiFiClient client;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // TX=17, RX=16

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to NTRIP caster
  Serial.println("Connecting to NTRIP caster...");
  if (!client.connect(ntripHost, ntripPort)) {
    Serial.println("Failed to connect to NTRIP caster.");
    return;
  }

  // Encode credentials
  String userpass = String(ntripUser) + ":" + String(ntripPass);
  unsigned char b64_output[128];
  size_t olen = 0;

  if (mbedtls_base64_encode(b64_output, sizeof(b64_output), &olen,
                            (const unsigned char*)userpass.c_str(), userpass.length()) != 0) {
    Serial.println("Base64 encoding failed");
    return;
  }

  String auth = "";
  for (size_t i = 0; i < olen; i++) {
    auth += (char)b64_output[i];
  }

  // Proper NTRIP request with Ntrip-Version header
String req  = "GET /" + String(mountpoint) + " HTTP/1.0\r\n";
req += "User-Agent: NTRIP ESP32/1.0\r\n";
req += "Authorization: " + auth + "\r\n";
req += "\r\n";

client.print(req);

  client.print(req);
  Serial.println("Sent NTRIP request:\n" + req);

  // Wait for response
  while (client.connected() && !client.available()) delay(100);

  // Read response headers
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
    return;
  }

  // Correct GGA sentence based on your location
  const char *ggaSentence = "$GPGGA,123519.00,5129.8838,N,00010.6243,W,1,12,1.0,53.4,M,0.0,M,,*4C\r\n";
  client.write((const uint8_t *)ggaSentence, strlen(ggaSentence));

  Serial.println("Receiving RTCM/NMEA data...");
}

void loop() {
  if (client.connected()) {
    while (client.available()) {
      uint8_t c = client.read();
      Serial.write(c);      // Debug output
      Serial2.write(c);     // Forward to GNSS or XBee
    }
  } else {
    Serial.println("NTRIP connection lost.");
    client.stop();
    delay(5000);
    ESP.restart();  // Reboot to try reconnect
  }
}
