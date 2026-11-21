#include "associazione.h"

#include <Arduino.h>
#include <esp_wifi.h>

#include "config.h"
#include "protocollo.h"
#include "tipo_antenna.h"

namespace associazione {
namespace {

constexpr int PIN_BOOT = 0;                 // pulsante BOOT del DevKit, attivo basso
constexpr uint32_t PRESSIONE_LUNGA_MS = 3000;
constexpr uint32_t ANTIRIMBALZO_MS = 30;
constexpr uint32_t INVIO_MS = 150;          // richieste durante la ricerca
constexpr uint32_t SOSTA_CANALE_MS = 450;   // tempo su ogni canale (3 richieste)
constexpr uint32_t INVIO_ATTESA_MS = 1000;  // richieste mentre si aspetta la conferma
constexpr uint32_t DURATA_MAX_MS = 60000;   // per la ricerca e per l'attesa della conferma
constexpr uint32_t ESITO_MS = 5000;         // esito sul display
constexpr uint8_t CANALI = 13;

Fase fase_ = Fase::INATTIVA;
uint8_t canale_ = 0;
uint8_t canaleIniziale = 1;
uint32_t inizioFase = 0, inizioCanale = 0, ultimoInvio = 0;
bool richiesta = false;  // da avvia()
bool breve = false;

// pulsante
bool premuto = false, lungaSegnalata = false;
uint32_t inizioPressione = 0;

enum class Evento : uint8_t { NESSUNO, BREVE, LUNGA };

Evento leggiPulsante() {
  bool giu = digitalRead(PIN_BOOT) == LOW;
  uint32_t ora = millis();
  if (giu && !premuto) {
    premuto = true;
    lungaSegnalata = false;
    inizioPressione = ora;
  } else if (giu && premuto && !lungaSegnalata && ora - inizioPressione >= PRESSIONE_LUNGA_MS) {
    lungaSegnalata = true;
    return Evento::LUNGA;
  } else if (!giu && premuto) {
    premuto = false;
    if (!lungaSegnalata && ora - inizioPressione >= ANTIRIMBALZO_MS) return Evento::BREVE;
  }
  return Evento::NESSUNO;
}

void impostaCanale(uint8_t c) {
  canale_ = c;
  esp_wifi_set_channel(c, WIFI_SECOND_CHAN_NONE);
}

void vaiIn(Fase f) {
  fase_ = f;
  inizioFase = millis();
}

void termina(Fase esito) {
  if (esito != Fase::RIUSCITA) impostaCanale(canaleIniziale);  // si torna dove si era
  vaiIn(esito);
}

void inviaRichiesta(Collegamento& radio) {
  proto::Associa a = {};
  a.versione = proto::VERSIONE;
  a.tipo = proto::Tipo::ASSOCIA_RICHIESTA;
  a.magia = proto::MAGIA_ASSOCIA;
  a.canale = canale_;
  a.flag = tipo_antenna::tx() ? proto::FLAG_VERSIONE_TX : 0;
  radio.inviaATutti(&a, sizeof(a));
  ultimoInvio = millis();
}

void parti() {
  uint8_t primario = 1;
  wifi_second_chan_t secondario;
  esp_wifi_get_channel(&primario, &secondario);
  canaleIniziale = primario;
  impostaCanale(1);
  inizioCanale = millis();
  ultimoInvio = 0;
  vaiIn(Fase::RICERCA);
  Serial.println("Associazione: ricerca del pannello su tutti i canali");
}

}  // namespace

void inizia() { pinMode(PIN_BOOT, INPUT_PULLUP); }

void avvia() { richiesta = true; }

bool aggiorna(Collegamento& radio, bool consentita, uint8_t mac[6], uint8_t& canale) {
  Evento ev = leggiPulsante();
  uint32_t ora = millis();
  bool cerca = fase_ == Fase::RICERCA || fase_ == Fase::ATTESA;

  if (!cerca) {
    if (ev == Evento::BREVE) breve = true;
    if ((ev == Evento::LUNGA || richiesta) && consentita) parti();
    else if (fase_ != Fase::INATTIVA && ora - inizioFase > ESITO_MS) vaiIn(Fase::INATTIVA);
    richiesta = false;
    ora = millis();  // parti() ha appena preso i suoi istanti
    if (fase_ != Fase::RICERCA) {
      Annuncio a;
      while (radio.riceviAnnuncio(a)) {}  // fuori dall'associazione si ignorano
      return false;
    }
  } else if (ev == Evento::BREVE) {
    Serial.println("Associazione annullata");
    termina(Fase::FALLITA);
    return false;
  }

  // risposte del pannello
  uint8_t mio[6];
  Collegamento::macLocale(mio);
  Annuncio a;
  while (radio.riceviAnnuncio(a)) {
    if (a.len != sizeof(proto::Associa)) continue;
    proto::Associa r;
    memcpy(&r, a.dati, sizeof(r));
    if (r.versione != proto::VERSIONE || r.magia != proto::MAGIA_ASSOCIA) continue;
    if (!Collegamento::stessoMac(r.destinatario, mio)) continue;
    if (r.tipo == proto::Tipo::ASSOCIA_ATTESA && fase_ == Fase::RICERCA) {
      vaiIn(Fase::ATTESA);  // si resta su questo canale
      Serial.printf("Associazione: pannello trovato sul canale %u, conferma sul pannello\n", canale_);
    } else if (r.tipo == proto::Tipo::ASSOCIA_CONFERMA) {
      memcpy(mac, a.mac, 6);
      canale = canale_;
      termina(Fase::RIUSCITA);
      return true;
    } else if (r.tipo == proto::Tipo::ASSOCIA_RIFIUTO) {
      Serial.println("Associazione rifiutata dal pannello");
      termina(Fase::RIFIUTATA);
      return false;
    }
  }

  if (ora - inizioFase > DURATA_MAX_MS) {
    Serial.println("Associazione: nessun pannello");
    termina(Fase::FALLITA);
    return false;
  }
  if (fase_ == Fase::RICERCA) {
    if (ora - inizioCanale >= SOSTA_CANALE_MS) {
      impostaCanale(canale_ % CANALI + 1);
      inizioCanale = ora;
      ultimoInvio = 0;
    }
    if (ultimoInvio == 0 || ora - ultimoInvio >= INVIO_MS) inviaRichiesta(radio);
  } else if (fase_ == Fase::ATTESA && ora - ultimoInvio >= INVIO_ATTESA_MS) {
    inviaRichiesta(radio);
  }
  return false;
}

Fase fase() { return fase_; }

uint8_t canale() { return canale_; }

bool pressioneBreve() {
  bool b = breve;
  breve = false;
  return b;
}

}  // namespace associazione
