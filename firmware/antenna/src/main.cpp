// Unità antenna: riceve i comandi (ESP-NOW o console seriale), muove il condensatore,
// gestisce calibrazione, azzeramento a ogni cambio banda e sicurezza dell'heartbeat.
#include <Arduino.h>
#include <Preferences.h>

#include "bande.h"
#include "calibrazione.h"
#include "collegamento.h"
#include "config.h"
#include "movimento.h"
#include "protocollo.h"
#include "segreti_scelti.h"

namespace {

Collegamento radio;
Preferences nvs;

uint16_t seqRisposta = 0;
uint32_t ultimoMessaggioRadio = 0;
bool movimentoDaRadio = false;  // il watchdog vale solo per i comandi via radio

bande::Banda bandaAzzerata = bande::Banda::NESSUNA;
bool targetInSospeso = false;  // target da raggiungere dopo l'azzeramento
int32_t targetSospeso = 0;
uint32_t freqCorrente = 0;
proto::Errore erroreComando = proto::Errore::NESSUNO;

proto::Stato statoUltimoInvio = proto::Stato::NON_AZZERATA;

// --------------------------------------------------------------------------------------------

proto::Errore erroreAttuale() {
  proto::Errore e = movimento::errore();
  return e != proto::Errore::NESSUNO ? e : erroreComando;
}

proto::Stato statoAttuale() {
  if (erroreAttuale() != proto::Errore::NESSUNO) return proto::Stato::ERRORE;
  if (movimento::inAzzeramento()) return proto::Stato::AZZERAMENTO;
  if (movimento::inMovimento() || targetInSospeso) return proto::Stato::IN_MOVIMENTO;
  if (!movimento::azzerato()) return proto::Stato::NON_AZZERATA;
  return proto::Stato::FERMO;
}

void inviaRisposta(uint16_t seqComando) {
  proto::Risposta r = {};
  r.versione = proto::VERSIONE;
  r.tipo = proto::Tipo::RISPOSTA;
  r.seq = ++seqRisposta;
  r.seq_comando = seqComando;
  r.stato = statoAttuale();
  r.errore = erroreAttuale();
  r.banda = (uint8_t)bande::di(freqCorrente);
  r.posizione = movimento::posizione();
  r.freq_hz = freqCorrente;
  radio.invia(&r, sizeof(r));
  statoUltimoInvio = r.stato;
}

void cancellaErrori() {
  erroreComando = proto::Errore::NESSUNO;
  movimento::cancellaErrore();
}

void vaiAFrequenza(uint32_t f) {
  bande::Banda b = bande::di(f);
  int32_t pos;
  if (b == bande::Banda::NESSUNA) {
    erroreComando = proto::Errore::FUORI_BANDA;
    return;
  }
  if (!calibrazione::posizionePer(f, pos)) {
    erroreComando = proto::Errore::NON_CALIBRATA;
    return;
  }
  freqCorrente = f;
  if (!movimento::azzerato() || b != bandaAzzerata) {
    // cambio banda (o posizione sconosciuta): prima l'azzeramento
    bandaAzzerata = b;
    targetSospeso = pos;
    targetInSospeso = true;
    movimento::azzera();
  } else {
    movimento::vaiA(pos);
  }
}

void esegui(const proto::Comando& c, bool daRadio) {
  switch (c.tipo) {
    case proto::Tipo::GOTO:
      if (movimento::inMovimento()) movimento::ferma();
      cancellaErrori();
      movimentoDaRadio = daRadio;
      vaiAFrequenza(c.freq_hz);
      break;

    case proto::Tipo::STOP:
      targetInSospeso = false;
      movimento::ferma();
      break;

    case proto::Tipo::ZERO:
      cancellaErrori();
      targetInSospeso = false;
      movimentoDaRadio = daRadio;
      movimento::azzera();
      break;

    case proto::Tipo::JOG:
      cancellaErrori();
      if (!movimento::azzerato() || movimento::inMovimento()) {
        erroreComando = proto::Errore::COMANDO_NON_VALIDO;
        break;
      }
      movimentoDaRadio = daRadio;
      freqCorrente = 0;  // la frequenza non è più nota
      movimento::vaiA(movimento::posizione() + c.valore);
      break;

    case proto::Tipo::CAL_SALVA:
      cancellaErrori();
      if (!movimento::azzerato() || movimento::inMovimento() ||
          !calibrazione::salvaPunto(c.freq_hz, movimento::posizione())) {
        erroreComando = proto::Errore::COMANDO_NON_VALIDO;
        break;
      }
      bandaAzzerata = bande::di(c.freq_hz);
      freqCorrente = c.freq_hz;
      break;

    case proto::Tipo::CAL_CANCELLA:
      calibrazione::cancella(bande::di(c.freq_hz));
      break;

    case proto::Tipo::STATO:
    case proto::Tipo::HEARTBEAT:
    default:
      break;
  }
  if (daRadio) inviaRisposta(c.seq);
}

// --- Radio ----------------------------------------------------------------------------------

void riceviRadio() {
  Pacchetto p;
  while (radio.ricevi(p)) {
    if (p.len != sizeof(proto::Comando)) continue;
    proto::Comando c;
    memcpy(&c, p.dati, sizeof(c));
    if (c.versione != proto::VERSIONE) continue;
    ultimoMessaggioRadio = millis();  // ogni comando vale come heartbeat
    esegui(c, true);
  }
}

void controllaHeartbeat() {
  if (!movimentoDaRadio || !movimento::inMovimento()) return;
  if (millis() - ultimoMessaggioRadio > cfg::TIMEOUT_HEARTBEAT_MS) {
    targetInSospeso = false;
    movimento::ferma();
    erroreComando = proto::Errore::HEARTBEAT;
    movimentoDaRadio = false;
  }
}

// --- Console seriale ------------------------------------------------------------------------

uint32_t leggiFrequenza(const char* s) {
  double v = atof(s);
  if (v <= 0) return 0;
  return v > 100000 ? (uint32_t)v : (uint32_t)lround(v * 1000.0);  // kHz se < 100000
}

void stampaStato() {
  Serial.printf("Stato: %s, errore: %s, posizione: %ld, freq: %.1f kHz, FC min/max: %d/%d\n",
                proto::nomeStato(statoAttuale()), proto::nomeErrore(erroreAttuale()),
                (long)movimento::posizione(), freqCorrente / 1000.0, movimento::finecorsaMin(),
                movimento::finecorsaMax());
}

void aiuto() {
  Serial.println(
      "Comandi: goto <kHz> | stop | zero | jog <passi> | cal <kHz> | calcanc <kHz> |\n"
      "         tabella | stato | mac <AA:BB:CC:DD:EE:FF> | aiuto");
}

void eseguiRiga(char* riga) {
  char* cmd = strtok(riga, " \t");
  char* arg = strtok(nullptr, " \t");
  if (!cmd) return;
  proto::Comando c = {};
  c.versione = proto::VERSIONE;

  if (!strcmp(cmd, "goto") && arg) {
    c.tipo = proto::Tipo::GOTO;
    c.freq_hz = leggiFrequenza(arg);
  } else if (!strcmp(cmd, "stop")) {
    c.tipo = proto::Tipo::STOP;
  } else if (!strcmp(cmd, "zero")) {
    c.tipo = proto::Tipo::ZERO;
  } else if (!strcmp(cmd, "jog") && arg) {
    c.tipo = proto::Tipo::JOG;
    c.valore = atol(arg);
  } else if (!strcmp(cmd, "cal") && arg) {
    c.tipo = proto::Tipo::CAL_SALVA;
    c.freq_hz = leggiFrequenza(arg);
  } else if (!strcmp(cmd, "calcanc") && arg) {
    c.tipo = proto::Tipo::CAL_CANCELLA;
    c.freq_hz = leggiFrequenza(arg);
  } else if (!strcmp(cmd, "tabella")) {
    calibrazione::stampa(Serial);
    return;
  } else if (!strcmp(cmd, "stato")) {
    stampaStato();
    return;
  } else if (!strcmp(cmd, "mac") && arg) {
    uint8_t mac[6];
    if (!Collegamento::leggiMac(arg, mac)) {
      Serial.println("MAC non valido");
      return;
    }
    nvs.putBytes("peer", mac, 6);
    Serial.println("MAC del controllo salvato: riavviare");
    return;
  } else {
    aiuto();
    return;
  }
  esegui(c, false);
  stampaStato();
}

void leggiConsole() {
  static char riga[64];
  static size_t n = 0;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      riga[n] = 0;
      n = 0;
      eseguiRiga(riga);
    } else if (n < sizeof(riga) - 1) {
      riga[n++] = ch;
    }
  }
}

}  // namespace

// --------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\nloopant - unità antenna (versione %s)\n", VERSIONE_TX ? "TX" : "RX");

  nvs.begin("loopant", false);
  movimento::inizia();
  calibrazione::inizia();

  uint8_t peer[6] = {0};
  nvs.getBytes("peer", peer, 6);
  bool ok = radio.inizia(peer, segreti::PMK, segreti::LMK, cfg::CANALE_WIFI);

  uint8_t mio[6];
  char testo[18];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, testo);
  Serial.printf("MAC di questa unità: %s\n", testo);
  if (!radio.peerValido())
    Serial.println("MAC del controllo non impostato: usare 'mac AA:BB:CC:DD:EE:FF'");
  else if (!ok)
    Serial.println("ERRORE: ESP-NOW non inizializzato");
  aiuto();
}

void loop() {
  riceviRadio();
  movimento::aggiorna();

  // fine azzeramento: si va al target in sospeso
  if (targetInSospeso && !movimento::inMovimento()) {
    targetInSospeso = false;
    if (movimento::azzerato() && movimento::errore() == proto::Errore::NESSUNO)
      movimento::vaiA(targetSospeso);
  }

  controllaHeartbeat();
  leggiConsole();

  // invio spontaneo quando cambia lo stato
  if (radio.peerValido() && statoAttuale() != statoUltimoInvio) inviaRisposta(0);
}
