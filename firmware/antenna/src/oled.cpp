#include "oled.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "config.h"

namespace oled {
namespace {

constexpr uint32_t PERIODO_MS = 250;          // al massimo 4 disegni al secondo
constexpr uint32_t SCHERMATA_AVVIO_MS = 4000;  // MAC e canale all'accensione

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, cfg::PIN_OLED_SCL, cfg::PIN_OLED_SDA);
portMUX_TYPE blocco = portMUX_INITIALIZER_UNLOCKED;
Dati dati = {};
bool nuovi = false;
uint32_t ultimaAttivita = 0;  // millis dell'ultimo cambio di stato o comando
bool presente_ = false;
char mac[18] = "";
uint8_t canale_ = 0;

// Il display può restare con SDA bassa se l'ESP32 si è riavviato a metà di un trasferimento:
// nove impulsi su SCL la liberano.
void sbloccaBus() {
  pinMode(cfg::PIN_OLED_SDA, INPUT_PULLUP);
  pinMode(cfg::PIN_OLED_SCL, OUTPUT_OPEN_DRAIN);
  for (int i = 0; i < 9; i++) {
    digitalWrite(cfg::PIN_OLED_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(cfg::PIN_OLED_SCL, HIGH);
    delayMicroseconds(5);
  }
  pinMode(cfg::PIN_OLED_SCL, INPUT_PULLUP);
}

bool diversi(const Dati& a, const Dati& b) {
  return a.stato != b.stato || a.errore != b.errore || a.freqHz != b.freqHz || a.posizione != b.posizione ||
         a.driver != b.driver || a.can != b.can || a.peerValido != b.peerValido || a.collegato != b.collegato ||
         a.rssi != b.rssi;
}

// Cambi che riaccendono il display (la posizione cambia solo durante un movimento, già segnalato dallo stato)
bool importanti(const Dati& a, const Dati& b) {
  return a.stato != b.stato || a.errore != b.errore || a.freqHz != b.freqHz || a.collegato != b.collegato ||
         a.peerValido != b.peerValido;
}

void accendi(bool si) {
  if (si) {
    u8g2.sendF("ca", 0x8D, 0x14);  // convertitore interno acceso
    u8g2.setPowerSave(0);
  } else {
    u8g2.setPowerSave(1);
    u8g2.sendF("ca", 0x8D, 0x10);  // convertitore interno spento: niente commutazioni a riposo
  }
}

const char* testoStato(proto::Stato s) {
  switch (s) {
    case proto::Stato::NON_AZZERATA: return "Non azzerata";
    case proto::Stato::FERMO: return "Fermo";
    case proto::Stato::IN_MOVIMENTO: return "In movimento";
    case proto::Stato::AZZERAMENTO: return "Azzeramento";
    case proto::Stato::ERRORE: return "Errore";
  }
  return "?";
}

void destra(int y, const char* t) { u8g2.drawUTF8(128 - u8g2.getUTF8Width(t), y, t); }

// Fascia gialla (righe 0-15): versione e collegamento
void fascia(const Dati& d) {
  u8g2.setFont(u8g2_font_6x13B_tf);
  u8g2.drawStr(0, 12, VERSIONE_TX ? "LOOPANT TX" : "LOOPANT RX");
  char t[16];
  if (!d.peerValido) snprintf(t, sizeof(t), "NO MAC");
  else if (!d.collegato) snprintf(t, sizeof(t), "SCOLLEGATO");
  else if (d.rssi) snprintf(t, sizeof(t), "%d dBm", d.rssi);
  else snprintf(t, sizeof(t), "COLLEGATO");
  destra(12, t);
}

void schermataAvvio(const Dati& d) {
  fascia(d);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(0, 27, "Unità antenna");
  char t[24];
  snprintf(t, sizeof(t), "MAC %s", mac);
  u8g2.drawStr(0, 39, t);
  snprintf(t, sizeof(t), "ESP-NOW canale %u", canale_);
  u8g2.drawStr(0, 51, t);
  u8g2.drawStr(0, 62, d.peerValido ? "Pannello impostato" : "Pannello assente");
}

void schermataSenzaPeer(const Dati& d) {
  fascia(d);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 27, "MAC del pannello");
  u8g2.drawStr(0, 39, "non impostato.");
  u8g2.drawUTF8(0, 51, "Questa unità:");
  u8g2.drawStr(0, 62, mac);
}

void schermataNormale(const Dati& d) {
  fascia(d);
  // frequenza in kHz con un decimale, cifre grandi
  char t[24];
  if (d.freqHz) snprintf(t, sizeof(t), "%lu,%lu", (unsigned long)(d.freqHz / 1000), (unsigned long)(d.freqHz % 1000 / 100));
  else snprintf(t, sizeof(t), "----,-");
  u8g2.setFont(u8g2_font_logisoso20_tn);
  u8g2.drawStr(0, 40, t);
  int x = u8g2.getStrWidth(t) + 4;
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(x, 40, "kHz");

  u8g2.drawStr(0, 51, testoStato(d.stato));
  if (d.stato == proto::Stato::ERRORE) {
    // il motivo al posto della posizione (tagliato a una riga)
    snprintf(t, sizeof(t), "%.21s", proto::nomeErrore(d.errore));
    u8g2.drawStr(0, 62, t);
    return;
  }
  snprintf(t, sizeof(t), "pos %ld", (long)d.posizione);
  u8g2.drawStr(0, 62, t);
  if (d.driver) destra(62, "MOTORE ON");
  else if (!VERSIONE_TX) destra(62, d.can ? "CAN" : "STEP/DIR");
}

void compito(void*) {
  const uint32_t avvio = millis();
  Dati disegnati = {};
  bool primo = true, acceso = true;
  for (;;) {
    portENTER_CRITICAL(&blocco);
    Dati d = dati;
    bool arrivati = nuovi;
    nuovi = false;
    uint32_t attivita = ultimaAttivita;
    portEXIT_CRITICAL(&blocco);

    bool inAvvio = millis() - avvio < SCHERMATA_AVVIO_MS;
    bool riposo = cfg::OLED_SPEGNI_S > 0 && !inAvvio && millis() - attivita > cfg::OLED_SPEGNI_S * 1000UL;
    if (acceso == riposo) {  // da spegnere o da riaccendere
      acceso = !riposo;
      accendi(acceso);
      primo = true;
    }
    if (acceso && (primo || inAvvio || (arrivati && diversi(d, disegnati)))) {
      u8g2.clearBuffer();
      if (inAvvio) schermataAvvio(d);
      else if (!d.peerValido) schermataSenzaPeer(d);
      else schermataNormale(d);
      u8g2.sendBuffer();
      disegnati = d;
      primo = false;
    }
    vTaskDelay(pdMS_TO_TICKS(PERIODO_MS));
  }
}

}  // namespace

bool inizia(const char* macLocale, uint8_t canale) {
  snprintf(mac, sizeof(mac), "%s", macLocale);
  canale_ = canale;
  sbloccaBus();
  Wire.begin(cfg::PIN_OLED_SDA, cfg::PIN_OLED_SCL, 400000);
  Wire.beginTransmission(cfg::OLED_INDIRIZZO);
  presente_ = Wire.endTransmission() == 0;
  if (!presente_) {
    Wire.end();
    return false;
  }
  u8g2.setI2CAddress(cfg::OLED_INDIRIZZO * 2);
  u8g2.setBusClock(400000);
  u8g2.begin();
  ultimaAttivita = millis();
  xTaskCreatePinnedToCore(compito, "oled", 4096, nullptr, 1, nullptr, 0);
  return true;
}

bool presente() { return presente_; }

void aggiorna(const Dati& d) {
  if (!presente_) return;
  portENTER_CRITICAL(&blocco);
  if (importanti(d, dati)) ultimaAttivita = millis();
  if (diversi(d, dati)) {
    dati = d;
    nuovi = true;
  }
  portEXIT_CRITICAL(&blocco);
}

void sveglia() {
  if (!presente_) return;
  portENTER_CRITICAL(&blocco);
  ultimaAttivita = millis();
  portEXIT_CRITICAL(&blocco);
}

}  // namespace oled
