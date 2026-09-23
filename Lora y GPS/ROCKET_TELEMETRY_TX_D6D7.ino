/*
 * ROCKET_TELEMETRY_TX.ino  —  XIAO ESP32-S3   (pines LoRa CORREGIDOS a D6/D7)
 * ================================================================
 * Lee el GPS ATGM336H (TinyGPS++) y manda la posicion por LoRa a 1 Hz.
 *
 * CAMBIOS vs la version que fallaba:
 *   1) LoRa en D6/D7 (no D2/D3). El modulo esta cableado a D6/D7 — lo
 *      probamos con ROCKET_LORA_TEST y funciono. Codigo y cable ahora
 *      COINCIDEN. Este era el bug: el sketch manejaba D2/D3 mientras el
 *      modulo estaba en D6/D7, asi que no le llegaba nada.
 *   2) Serial.setTxTimeoutMs(0): el debug USB nunca bloquea el loop
 *      cuando nadie lo lee (powerbank). Antes podia atorar el envio.
 *
 * El LR02 ya quedo configurado en flash:
 *   915.000 MHz (canal 82) | SF10 | BW 125k | CR 4/5 | CRC on
 *   MODE 0 (transparente)  | SLEEP 2 | 22 dBm | UART 9600 8N1
 * Este sketch NO manda comandos AT: escribe texto al UART y el
 * modulo lo saca por RF tal cual.
 *
 * ---------------- UARTs ----------------
 *   UART1 -> GPS   (D5/D4)
 *   UART2 -> LoRa  (D7/D6)
 *   Serial (USB)  -> debug
 * OJO: no se usa el nombre "Serial1" en ningun lado. Serial1 ES UART1,
 * o sea el GPS. Usar los dos nombres para cosas distintas los revienta.
 *
 * ---------------- CABLEADO ----------------
 *  ATGM336H                  XIAO ESP32-S3
 *   VCC          ------->    3V3
 *   GND          ------->    GND
 *   TX           ------->    D5        (RX del XIAO)
 *   RX           ------->    D4        (TX del XIAO)
 *
 *  LR02                      XIAO ESP32-S3
 *   VCC (pin 6)  ------->    3V3
 *   GND (pin 7)  ------->    GND
 *   UART_RX (3)  ------->    D6        (TX del XIAO)
 *   UART_TX (4)  ------->    D7        (RX del XIAO)
 *   M0, M1       ------->    sin conectar
 *
 *  IMPORTANTE:
 *   - 100uF + 0.1uF lo mas cerca posible del VCC del LR02.
 *   - Antena SIEMPRE puesta antes de energizar.
 *   - Separa la antena ceramica del GPS de la antena LoRa.
 *
 * ---------------- PAQUETE ----------------
 *   R,<seq>,<lat>,<lon>,<alt>,<sats>,<fix>\n
 *   ej: R,0042,20.588412,-100.389755,1823.4,09,1
 */

#include <TinyGPSPlus.h>

// ---------------- pines ----------------
#define GPS_RX   D5        // <- GPS TX
#define GPS_TX   D4        // -> GPS RX
#define LORA_RX  D7        // <- LR02 UART_TX (pin 4)
#define LORA_TX  D6        // -> LR02 UART_RX (pin 3)

// ---------------- config ----------------
#define SEND_INTERVAL_MS  1000
#define FIX_MAX_AGE_MS    5000    // arriba de esto la posicion se considera vieja
#define DEBUG_USB         1       // 1 = imprime por USB para probar en banca
#define DEBUG_NMEA        0       // 1 = vuelca el NMEA crudo (ruidoso)

TinyGPSPlus    gps;
HardwareSerial gpsSerial(1);      // UART1 — GPS
HardwareSerial loraSerial(2);     // UART2 — LoRa

uint32_t seq = 0, lastSend = 0;

void sendPacket() {
  bool  ok   = gps.location.isValid() && gps.location.age() < FIX_MAX_AGE_MS;
  double lat = ok ? gps.location.lat() : 0.0;
  double lon = ok ? gps.location.lng() : 0.0;
  float  alt = gps.altitude.isValid() ? gps.altitude.meters() : 0.0f;
  uint8_t sats = gps.satellites.isValid() ? gps.satellites.value() : 0;

  char pkt[64];
  int n = snprintf(pkt, sizeof(pkt), "R,%04lu,%.6f,%.6f,%.1f,%02u,%u\n",
                   (unsigned long)(seq++ % 10000), lat, lon, alt, sats, ok ? 1 : 0);
  loraSerial.write((const uint8_t*)pkt, n);

#if DEBUG_USB
  Serial.print(pkt);
#endif
}

void setup() {
#if DEBUG_USB
  Serial.begin(115200);
  #if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);   // el debug USB nunca bloquea el loop (powerbank)
  #endif
  delay(1500);
  Serial.println("\nROCKET TELEMETRY TX");
  Serial.println("GPS ATGM336H (UART1 D5/D4) -> LoRa 915.000 MHz ch82 SF10 (UART2 D6/D7)");
#endif
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);
  lastSend = millis();
}

void loop() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);
#if DEBUG_NMEA
    Serial.write(c);
#endif
  }

  uint32_t now = millis();
  if (now - lastSend >= SEND_INTERVAL_MS) {
    lastSend += SEND_INTERVAL_MS;
    sendPacket();
  }
}
