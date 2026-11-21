// Prova del transceiver CAN (SN65HVD230) senza altri nodi: TWAI in modalità NO_ACK con autoricezione.
// Il messaggio esce da CTX, passa dal transceiver e rientra da CRX: se torna, cablaggio e alimentazione sono a posto.
#include <driver/twai.h>

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== prova_can (CTX GPIO 17, CRX GPIO 16, 500 kbit/s) ==");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_17, GPIO_NUM_16, TWAI_MODE_NO_ACK);
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
    Serial.println("ERRORE: driver TWAI non avviato");
    return;
  }
  int ricevuti = 0, inviati = 0;
  for (int i = 0; i < 10; i++) {
    twai_message_t m = {};
    m.identifier = 0x7F0;  // non è l'indirizzo del SERVO42D
    m.data_length_code = 2;
    m.data[0] = 0xAA;
    m.data[1] = (uint8_t)i;
    m.self = 1;
    if (twai_transmit(&m, pdMS_TO_TICKS(100)) != ESP_OK) continue;
    inviati++;
    twai_message_t r;
    if (twai_receive(&r, pdMS_TO_TICKS(100)) == ESP_OK && r.identifier == 0x7F0 && r.data[1] == i) ricevuti++;
  }
  delay(50);
  twai_status_info_t s;
  twai_get_status_info(&s);
  const char* stati[] = {"fermo", "attivo", "bus-off", "in recupero"};
  Serial.printf("Inviati %d/10, tornati %d/10 · bus %s · errori TX %lu, RX %lu, bus %lu\n", inviati, ricevuti,
                s.state <= 3 ? stati[s.state] : "?", (unsigned long)s.tx_error_counter,
                (unsigned long)s.rx_error_counter, (unsigned long)s.bus_error_count);
  Serial.println(ricevuti == 10 ? "ESITO: OK, CTX/CRX e transceiver funzionano"
                                : "ESITO: NON OK (controllare 3V3/GND, CTX->17, CRX->16, non incrociati)");
}

void loop() {}
