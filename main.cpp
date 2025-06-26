#include <Arduino.h>
#include <HardwareSerial.h>

// Use UART2 on ESP32 (GPIO 16 = RX2, GPIO 17 = TX2 by default)
HardwareSerial GPS_Serial(2);

String nmea = "";

float convertToDecimal(float val) {
  int deg = int(val / 100);
  float minutes = val - (deg * 100);
  return deg + (minutes / 60.0);
}


void parseLatLng(String sentence) {
  // Split the sentence into fields using commas
  int index = 0;
  int from = 0;
  String fields[10]; // we only care up to field 5

  for (int i = 0; i < sentence.length(); i++) {
    if (sentence[i] == ',' || i == sentence.length() - 1) {
      fields[index] = sentence.substring(from, i);
      from = i + 1;
      index++;
      if (index >= 10) break;
    }
  }

  String rawLat = fields[2];
  String latDir = fields[3];
  String rawLng = fields[4];
  String lngDir = fields[5];
  String rawAlt = fields[9];

  if (rawLat != "" && rawLng != "") {
    float lat = convertToDecimal(rawLat.toFloat());
    float lng = convertToDecimal(rawLng.toFloat());

    if (latDir == "S") lat = -lat;
    if (lngDir == "W") lng = -lng;

    float altitude = rawAlt.toFloat();

    Serial.print("Latitude: ");
    Serial.println(lat, 6);
    Serial.print("Longitude: ");
    Serial.println(lng, 6);
    Serial.print("Altitude: ");
    Serial.print(altitude, 2);
    Serial.println(" m");
  }
}

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  while (!Serial);

  // Start GPS serial port (baud rate depends on your GPS module, often 9600 or 38400)
  GPS_Serial.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

  Serial.println("Reading NMEA sentences from GPS...");
}

void loop() {
  // Read from GPS and print to Serial Monitor
  while (GPS_Serial.available()) {
    char c = GPS_Serial.read();
    if (c == '\n') {
      // Serial.println("Got full NMEA sentence:");
      if (nmea.startsWith("$GNGGA")) {
        parseLatLng(nmea);
      }
      nmea = ""; // reset for next line
    } else {
      nmea += c;
    }
  }
}
