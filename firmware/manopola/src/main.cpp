// Manopola remota loopant: decoder CW/RTTY dal microfono, ritocco fine, frequenza e TUNE, sempre attraverso il
// pannello (che comanda l'antenna). Collegamento ESP-NOW cifrato con il pannello, associazione e ricerca del
// canale come l'antenna. Scheda: Waveshare ESP32-S3-Knob-Touch-LCD-1.8.
#include <Arduino.h>
#include <Preferences.h>
#include <associazione.h>
#include <collegamento.h>
#include <protocollo.h>
#include <ricerca_canale.h>
#include <segreti_scelti.h>

#include "condiviso.h"
#include "config.h"
#include "decodifica.h"
#include "encoder.h"
#include "interfaccia.h"
#include "waveshare/cst816.h"
#include "waveshare/lcd_bl_pwm_bsp.h"
#include "waveshare/schermo_lcd.h"

namespace {

constexpr uint32_t SILENZIO_MS = 3000;             // pannello assente dopo 3 s senza impostazioni
constexpr uint32_t RICERCA_SILENZIO_MS = 5000;     // poi si cerca il suo canale
constexpr uint32_t RICERCA_INTERVALLO_MS = 10000;
constexpr uint32_t ASSOCIA_INTERVALLO_MS = 15000;  // non associata: nuova ricerca del pannello
constexpr uint32_t STATO_MS = 100;

Collegamento radio;
Preferences nvs;
uint8_t canale = cfg::CANALE_WIFI;
uint32_t ultimoPannello = 0, ultimoStato = 0, ultimaRicerca = 0, ultimaAssociazione = 0;
int8_t rssi = 0;
proto::ManopolaImposta imp = {};
bool impValida = false;

void riceviPannello() {
  Pacchetto p;
  while (radio.ricevi(p)) {
    if (p.len != sizeof(proto::ManopolaImposta)) continue;
    proto::ManopolaImposta r;
    memcpy(&r, p.dati, sizeof(r));
    if (r.versione != proto::VERSIONE || r.tipo != proto::Tipo::MANOPOLA_IMPOSTA) continue;
    imp = r;
    impValida = true;
    ultimoPannello = millis();
    rssi = p.rssi;
    decodifica::configura(r);
  }
}

proto::ManopolaStato preparaStato() {
  proto::ManopolaStato s = {};
  s.versione = proto::VERSIONE;
  s.tipo = proto::Tipo::MANOPOLA_STATO;
  decodifica::riempi(s);
  condiviso::Comandi c = condiviso::comandi();
  s.scatti = c.scatti;
  s.nTune = c.nTune;
  s.freqTune = c.freqTune;
  s.nZero = c.nZero;
  return s;
}

void inviaStato() {
  if (!radio.peerValido() || associazione::fase() != associazione::Fase::INATTIVA) return;
  if (millis() - ultimoStato < STATO_MS) return;
  ultimoStato = millis();
  proto::ManopolaStato s = preparaStato();
  radio.invia(&s, sizeof(s));
}

void cercaCanale() {
  if (!radio.peerValido() || associazione::fase() != associazione::Fase::INATTIVA) return;
  uint32_t ora = millis();
  if (ora - ultimoPannello < RICERCA_SILENZIO_MS) return;  // 0 = dall'accensione
  if (ultimaRicerca && ora - ultimaRicerca < RICERCA_INTERVALLO_MS) return;
  ultimaRicerca = ora;
  proto::ManopolaStato s = preparaStato();
  uint8_t prima = canale;
  if (ricerca_canale::cerca(radio, canale, &s, sizeof(s), 40) && canale != prima) {
    Serial.printf("Pannello trovato sul canale %u (prima %u): salvato\n", canale, prima);
    nvs.putUChar("canale", canale);
  }
}

void aggiornaAssociazione() {
  uint8_t mac[6], ch;
  if (associazione::aggiorna(radio, true, mac, ch)) {
    nvs.putBytes("peer", mac, 6);
    nvs.putUChar("canale", ch);
    canale = ch;
    radio.cambiaPeer(mac, segreti::LMK);
    ultimoPannello = millis();
    char t[18];
    Collegamento::scriviMac(mac, t);
    Serial.printf("Associata al pannello %s sul canale %u (salvato)\n", t, ch);
  }
  // non associata: cerca il pannello da sola, ogni tanto (niente pulsante: la conferma è sul pannello)
  if (!radio.peerValido() && associazione::fase() == associazione::Fase::INATTIVA &&
      (!ultimaAssociazione || millis() - ultimaAssociazione > ASSOCIA_INTERVALLO_MS)) {
    ultimaAssociazione = millis();
    associazione::avvia();
  }
}

void pubblica() {
  condiviso::Pannello p = {};
  p.collegato = ultimoPannello && millis() - ultimoPannello < SILENZIO_MS;
  p.associato = radio.peerValido();
  p.rssi = rssi;
  p.associazione = (uint8_t)associazione::fase();
  p.canale = associazione::fase() == associazione::Fase::INATTIVA ? canale : associazione::canale();
  p.valida = impValida && p.collegato;
  p.imp = imp;
  condiviso::pannello(p);
}

void console() {
  static char riga[48];
  static size_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (n < sizeof(riga) - 1) riga[n++] = c;
      continue;
    }
    riga[n] = 0;
    n = 0;
    char* cmd = strtok(riga, " ");
    char* arg = strtok(nullptr, " ");
    if (!cmd) continue;
    if (!strcmp(cmd, "associa")) {
      associazione::avvia();
    } else if (!strcmp(cmd, "dimentica")) {
      uint8_t zero[6] = {0};
      nvs.putBytes("peer", zero, 6);
      radio.cambiaPeer(zero, segreti::LMK);
      Serial.println("Pannello dimenticato");
    } else if (!strcmp(cmd, "canale") && arg) {
      int ch = atoi(arg);
      if (ch >= 1 && ch <= 13) {
        nvs.putUChar("canale", (uint8_t)ch);
        Serial.printf("Canale %d salvato: riavviare\n", ch);
      }
    } else if (!strcmp(cmd, "stato")) {
      condiviso::Pannello p = condiviso::pannello();
      condiviso::Comandi k = condiviso::comandi();
      Serial.printf("Pannello: %s%s, canale %u, %d dBm · scatti %ld · modo %s · sorgente %u · livello %.3f\n",
                    p.associato ? "associato" : "non associato", p.collegato ? ", collegato" : "", p.canale, p.rssi,
                    (long)k.scatti, k.modoFrequenza ? "frequenza" : "ritocco", (unsigned)decodifica::sorgente(),
                    decodifica::livello());
    } else {
      Serial.println("Comandi: stato | associa | dimentica | canale <1-13>");
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Touch_Init();
  lcd_lvgl_Init();
  lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
  encoder::inizia();
  decodifica::inizia();

  nvs.begin("manopola", false);
  uint8_t peer[6] = {0};
  nvs.getBytes("peer", peer, 6);
  canale = nvs.getUChar("canale", cfg::CANALE_WIFI);
  if (canale < 1 || canale > 13) canale = cfg::CANALE_WIFI;
  radio.inizia(peer, segreti::PMK, segreti::LMK, canale);
  associazione::inizia(-1, proto::FLAG_MANOPOLA);  // niente pulsante

  if (example_lvgl_lock(-1)) {
    interfaccia::crea();
    example_lvgl_unlock();
  }
  uint8_t mio[6];
  char t[18];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, t);
  Serial.printf("\nloopant - manopola remota, MAC %s, canale %u, %s\n", t, canale,
                radio.peerValido() ? "pannello associato" : "pannello non associato: lo cerco");
}

void loop() {
  riceviPannello();
  aggiornaAssociazione();
  cercaCanale();
  inviaStato();
  pubblica();
  console();
  delay(5);
}
