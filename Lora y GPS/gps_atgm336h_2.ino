#include <TinyGPSPlus.h>

#define GPS_RX D5   // XIAO D5  <- GPS TX
#define GPS_TX D4   // XIAO D4  -> GPS RX

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("Waiting for GPS fix...");
}

void loop() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    Serial.write(c);        // raw NMEA
    gps.encode(c);
  }

  if (gps.location.isUpdated()) {
    Serial.printf("\n>>> Lat: %.6f  Lon: %.6f  Sats: %d  Alt: %.1f m\n",
                  gps.location.lat(), gps.location.lng(),
                  gps.satellites.value(), gps.altitude.meters());
  }
}