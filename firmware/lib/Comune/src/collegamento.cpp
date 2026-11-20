#include "collegamento.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/queue.h>

namespace {
QueueHandle_t coda = nullptr;
uint8_t macPeer[6] = {0};
bool peerOk = false;

void elabora(const uint8_t* mac, const uint8_t* dati, int len) {
  if (!coda || len <= 0 || len > (int)sizeof(Pacchetto::dati)) return;
  if (memcmp(mac, macPeer, 6) != 0) return;  // solo dal peer configurato
  Pacchetto p;
  p.len = (uint8_t)len;
  memcpy(p.dati, dati, len);
  xQueueSend(coda, &p, 0);
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void suRicezione(const esp_now_recv_info_t* info, const uint8_t* dati, int len) {
  elabora(info->src_addr, dati, len);
}
#else
void suRicezione(const uint8_t* mac, const uint8_t* dati, int len) { elabora(mac, dati, len); }
#endif

bool macNullo(const uint8_t m[6]) {
  for (int i = 0; i < 6; i++)
    if (m[i]) return false;
  return true;
}
}  // namespace

bool Collegamento::inizia(const uint8_t peer[6], const uint8_t pmk[16], const uint8_t lmk[16],
                          uint8_t canale) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(canale, WIFI_SECOND_CHAN_NONE);

  if (!coda) coda = xQueueCreate(8, sizeof(Pacchetto));
  if (esp_now_init() != ESP_OK) return false;
  esp_now_set_pmk(pmk);
  esp_now_register_recv_cb(suRicezione);

  memcpy(macPeer, peer, 6);
  peerOk = false;
  if (macNullo(peer)) return true;  // peer da impostare con il comando "mac"

  esp_now_peer_info_t info = {};
  memcpy(info.peer_addr, peer, 6);
  info.channel = canale;
  info.ifidx = WIFI_IF_STA;
  info.encrypt = true;
  memcpy(info.lmk, lmk, 16);
  peerOk = esp_now_add_peer(&info) == ESP_OK;
  return peerOk;
}

bool Collegamento::invia(const void* dati, size_t len) {
  if (!peerOk) return false;
  return esp_now_send(macPeer, (const uint8_t*)dati, len) == ESP_OK;
}

bool Collegamento::ricevi(Pacchetto& p) { return coda && xQueueReceive(coda, &p, 0) == pdTRUE; }

bool Collegamento::peerValido() const { return peerOk; }

void Collegamento::macLocale(uint8_t out[6]) { esp_wifi_get_mac(WIFI_IF_STA, out); }

bool Collegamento::leggiMac(const char* testo, uint8_t out[6]) {
  unsigned v[6];
  if (sscanf(testo, "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6)
    return false;
  for (int i = 0; i < 6; i++) {
    if (v[i] > 0xFF) return false;
    out[i] = (uint8_t)v[i];
  }
  return true;
}

void Collegamento::scriviMac(const uint8_t mac[6], char out[18]) {
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
           mac[5]);
}
