// Unità antenna: riceve i comandi (ESP-NOW o console seriale), muove il condensatore,
// gestisce calibrazione, azzeramento a ogni cambio banda e sicurezza dell'heartbeat.
#include <Arduino.h>
#include <Preferences.h>

#include "associazione.h"
#include "bande.h"
#include "calibrazione.h"
#include "collegamento.h"
#include "config.h"
#include "movimento.h"
#include "oled.h"
#include "protocollo.h"
#include "segreti_scelti.h"

namespace {

Collegamento radio;
Preferences nvs;
uint8_t macPannello[6] = {0};
uint8_t canale = cfg::CANALE_WIFI;  // canale ESP-NOW (NVS "canale"): quello del router se il pannello usa il WiFi

uint16_t seqRisposta = 0;
uint32_t ultimoMessaggioRadio = 0;
int8_t ultimoRssi = 0;
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
  r.flag = (movimento::finecorsaMin() ? proto::FLAG_FC_MIN : 0) |
           (movimento::finecorsaMax() ? proto::FLAG_FC_MAX : 0) |
           (movimento::driverAbilitato() ? proto::FLAG_DRIVER : 0) |
           (movimento::modo() == movimento::Modo::CAN ? proto::FLAG_CAN : 0) |
           (VERSIONE_TX ? proto::FLAG_VERSIONE_TX : 0) |
           (movimento::azzerato() ? proto::FLAG_AZZERATA : 0);
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

void inviaPunto(const proto::Comando& c) {
  proto::RispostaCal r = {};
  r.versione = proto::VERSIONE;
  r.tipo = proto::Tipo::RISPOSTA_CAL;
  r.seq = ++seqRisposta;
  r.seq_comando = c.seq;
  bande::Banda b = bande::di(c.freq_hz);
  r.banda = (uint8_t)b;
  r.totale = calibrazione::numeroPunti(b);
  r.indice = (uint8_t)c.valore;
  uint32_t f = 0;
  int32_t pos = 0;
  calibrazione::punto(b, r.indice, f, pos);
  r.freq_hz = f;  // campi packed: niente riferimenti diretti
  r.posizione = pos;
  radio.invia(&r, sizeof(r));
}

void esegui(const proto::Comando& c, bool daRadio) {
  if (c.tipo == proto::Tipo::CAL_LEGGI) {  // risponde con il punto, non con lo stato
    if (daRadio) inviaPunto(c);
    return;
  }
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

    case proto::Tipo::CAL_ELIMINA:
      cancellaErrori();
      if (!calibrazione::eliminaPunto(c.freq_hz)) erroreComando = proto::Errore::COMANDO_NON_VALIDO;
      break;

    case proto::Tipo::IMPOSTA: {
      cancellaErrori();
      bool ok = false;
      switch ((proto::Parametro)c.freq_hz) {
        case proto::Parametro::MODO_MOTORE:
          ok = movimento::impostaModo(c.valore == 1 ? movimento::Modo::CAN : movimento::Modo::STEP_DIR);
          if (ok) targetInSospeso = false;  // con l'altro driver serve un nuovo azzeramento
          break;
        case proto::Parametro::CAN_ID:
          ok = movimento::impostaCan((uint16_t)c.valore, movimento::canKbit());
          break;
        case proto::Parametro::CAN_KBIT:
          ok = movimento::impostaCan(movimento::canId(), (uint16_t)c.valore);
          break;
      }
      if (!ok) erroreComando = proto::Errore::COMANDO_NON_VALIDO;
      break;
    }

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
    ultimoRssi = p.rssi;
    if (c.tipo != proto::Tipo::HEARTBEAT && c.tipo != proto::Tipo::STATO) oled::sveglia();
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

// --- Associazione con il pulsante BOOT ---------------------------------------------------------

void aggiornaAssociazione() {
  uint8_t mac[6];
  uint8_t ch;
  bool consentita = !movimento::inMovimento() && !targetInSospeso;
  if (associazione::aggiorna(radio, consentita, mac, ch)) {
    nvs.putBytes("peer", mac, 6);
    nvs.putUChar("canale", ch);
    memcpy(macPannello, mac, 6);
    canale = ch;
    radio.cambiaPeer(mac, segreti::LMK);
    ultimoMessaggioRadio = 0;
    char t[18];
    Collegamento::scriviMac(mac, t);
    Serial.printf("Associata al pannello %s sul canale %u (salvato)\n", t, ch);
  }
  if (associazione::pressioneBreve()) oled::sveglia();
}

// --- Display OLED ---------------------------------------------------------------------------

void aggiornaOled() {
  static uint32_t ultimo = 0;
  if (millis() - ultimo < 100) return;
  ultimo = millis();
  oled::Dati d = {};
  d.stato = statoAttuale();
  d.errore = erroreAttuale();
  d.freqHz = freqCorrente;
  d.posizione = movimento::posizione();
  d.driver = movimento::driverAbilitato();
  d.can = movimento::modo() == movimento::Modo::CAN;
  d.peerValido = radio.peerValido();
  d.collegato = ultimoMessaggioRadio != 0 && millis() - ultimoMessaggioRadio < cfg::TIMEOUT_HEARTBEAT_MS;
  d.rssi = ultimoRssi;
  d.associa = (uint8_t)associazione::fase();
  d.canale = associazione::fase() == associazione::Fase::INATTIVA ? canale : associazione::canale();
  if (radio.peerValido()) Collegamento::scriviMac(macPannello, d.pannello);
  oled::aggiorna(d);
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
  Serial.printf("ESP-NOW: canale %u\n", canale);
  if (movimento::modo() == movimento::Modo::CAN)
    Serial.printf("Motore: CAN (SERVO42D indirizzo %u, %u kbit/s)\n", movimento::canId(), movimento::canKbit());
  else
    Serial.println("Motore: STEP/DIR");
}

void aiuto() {
  Serial.println(
      "Comandi: goto <kHz> | stop | zero | jog <passi> | cal <kHz> | calelim <kHz> | calcanc <kHz> |\n"
      "         tabella | stato | mac <AA:BB:CC:DD:EE:FF> | canale <1-13> | associa |\n"
      "         motore stepdir|can | canid <1-2047> | cankbit <125|250|500|1000> (solo RX) | aiuto");
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
  } else if (!strcmp(cmd, "motore") && arg) {
    c.tipo = proto::Tipo::IMPOSTA;
    c.freq_hz = (uint32_t)proto::Parametro::MODO_MOTORE;
    c.valore = !strcmp(arg, "can") ? 1 : 0;
  } else if (!strcmp(cmd, "canid") && arg) {
    c.tipo = proto::Tipo::IMPOSTA;
    c.freq_hz = (uint32_t)proto::Parametro::CAN_ID;
    c.valore = atol(arg);
  } else if (!strcmp(cmd, "cankbit") && arg) {
    c.tipo = proto::Tipo::IMPOSTA;
    c.freq_hz = (uint32_t)proto::Parametro::CAN_KBIT;
    c.valore = atol(arg);
  } else if (!strcmp(cmd, "calelim") && arg) {
    c.tipo = proto::Tipo::CAL_ELIMINA;
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
  } else if (!strcmp(cmd, "associa")) {
    associazione::avvia();  // come BOOT premuto 3 s
    return;
  } else if (!strcmp(cmd, "canale") && arg) {
    int n = atoi(arg);
    if (n < 1 || n > 13) {
      Serial.println("Canale non valido (1-13)");
      return;
    }
    nvs.putUChar("canale", (uint8_t)n);
    Serial.printf("Canale %d salvato: riavviare\n", n);
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
  associazione::inizia();

  uint8_t peer[6] = {0};
  nvs.getBytes("peer", peer, 6);
  memcpy(macPannello, peer, 6);
  canale = nvs.getUChar("canale", cfg::CANALE_WIFI);
  if (canale < 1 || canale > 13) canale = cfg::CANALE_WIFI;
  bool ok = radio.inizia(peer, segreti::PMK, segreti::LMK, canale);

  uint8_t mio[6];
  char testo[18];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, testo);
  Serial.printf("MAC di questa unità: %s\n", testo);
  Serial.printf("Display OLED: %s\n", oled::inizia(testo, canale) ? "presente" : "non trovato (SDA 21, SCL 22)");
  if (!radio.peerValido())
    Serial.println("Pannello non associato: tenere premuto BOOT per 3 s, oppure 'mac AA:BB:CC:DD:EE:FF'");
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
  aggiornaAssociazione();
  aggiornaOled();

  // invio spontaneo quando cambia lo stato
  if (radio.peerValido() && statoAttuale() != statoUltimoInvio) inviaRisposta(0);
}
