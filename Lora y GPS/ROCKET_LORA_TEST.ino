/*
 * ROCKET_LORA_TEST.ino  —  XIAO ESP32-S3
 * ================================================================
 * Prueba SOLO del enlace LoRa: módulo + cableado XIAO->LR02 + antena +
 * alimentación por powerbank.  NO usa el GPS.
 *
 * Manda un paquete "R,..." de prueba cada segundo con coordenadas FIJAS
 * (un punto en Querétaro) y una altitud que sube, para que se vea llegar
 * COMPLETO en la Estación de Tierra: sube RECIBIDOS, se mueve la gráfica
 * de altitud y aparece el punto en el mapa. Camina con el powerbank y
 * vigila PERDIDOS = tu prueba de alcance/antena.
 *
 * El LED de a bordo parpadea cada paquete = "sigo vivo" (útil en powerbank).
 *
 * >>> IMPORTANTE: pon LORA_TX / LORA_RX IGUAL a tu cableado REAL <<<
 *   LR02 UART_RX (pin 3)  <-  LORA_TX   (el XIAO TRANSMITE por aquí)
 *   LR02 UART_TX (pin 4)  ->  LORA_RX   (el XIAO RECIBE por aquí)
 *   LR02 VCC (pin 6) -> 3V3   |   LR02 GND (pin 7) -> GND (común con el XIAO)
 *   Antena SIEMPRE puesta antes de energizar.
 *   100uF + 0.1uF lo más cerca posible del VCC del LR02.
 *
 * El LR02 ya está en flash: 915.000 MHz (canal 82) | SF10 | BW125k | CR4/5
 *   | CRC on | MODE0 transparente | SLEEP2 | 22 dBm | UART 9600 8N1.
 */

#include <Arduino.h>

// >>> AJUSTA ESTOS DOS A TU CABLEADO FÍSICO <<<
// La versión que funcionó usaba D6/D7. Si moviste los cables a D2/D3,
// cámbialos aquí — pero código y cable DEBEN coincidir.
#define LORA_TX   D6      // -> LR02 pin 3 (UART_RX)
#define LORA_RX   D7      // <- LR02 pin 4 (UART_TX)

#define LORA_BAUD 9600
#define SEND_MS   1000

HardwareSerial loraSerial(2);   // UART2 — OJO: nunca uses el nombre "Serial1"
uint32_t seq = 0, lastSend = 0;

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);     // USB nunca bloquea el loop si nadie lee (powerbank)
#endif
  pinMode(LED_BUILTIN, OUTPUT);

  loraSerial.begin(LORA_BAUD, SERIAL_8N1, LORA_RX, LORA_TX);
  delay(300);
  Serial.println("\nROCKET LORA TEST — solo LoRa, SIN GPS");
  Serial.printf("UART2 LoRa: TX=D%d (-> LR02 pin3)  RX=D%d (<- LR02 pin4)  %d 8N1\n",
                LORA_TX, LORA_RX, LORA_BAUD);
  Serial.println("Debe llegar a la Estación de Tierra como paquetes R,... (fix=1).");
  lastSend = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastSend >= SEND_MS) {
    lastSend += SEND_MS;

    float alt = 1000.0f + (float)(seq % 300);   // 1000..1300 m, se ve moverse la gráfica
    char pkt[64];
    int n = snprintf(pkt, sizeof(pkt),
                     "R,%04lu,20.588412,-100.389755,%.1f,08,1\n",
                     (unsigned long)(seq++ % 10000), alt);

    loraSerial.write((const uint8_t*)pkt, n);    // -> al aire por RF
    Serial.print(pkt);                            // -> debug USB (solo en banca)
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // parpadeo = vivo
  }
}
