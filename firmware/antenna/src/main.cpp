// Unità antenna: riceve i comandi (ESP-NOW o console seriale), muove il condensatore,
// gestisce calibrazione, azzeramento a ogni cambio banda e sicurezza dell'heartbeat.
#include <Arduino.h>
#include <Preferences.h>
#include <esp_wifi.h>

#include "associazione.h"
#include "bande.h"
#include "calibrazione.h"
#include "collegamento.h"
#include "config.h"
#include "movimento.h"
#include "oled.h"
#include "protocollo.h"
#include "ricerca_canale.h"
#include "tipo_antenna.h"
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
bool riavvioRichiesto = false;  // cambio del tipo di antenna: riavvio dopo la risposta

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

proto::Risposta preparaRisposta(uint16_t seqComando) {
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
           (movimento::viaCan() ? proto::FLAG_CAN : 0) |
           (tipo_antenna::tx() ? proto::FLAG_VERSIONE_TX : 0) |
           (movimento::azzerato() ? proto::FLAG_AZZERATA : 0);
  return r;
}

void inviaRisposta(uint16_t seqComando) {
  proto::Risposta r = preparaRisposta(seqComando);
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
      movimento::vaiA(movimento::posizione() + c.valore, c.freq_hz != proto::JOG_LIBERO);
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
        case proto::Parametro::MODO_MOTORE:  // non più usato: il driver segue il tipo di antenna
          ok = (c.valore == 1) == movimento::viaCan();
          break;
        case proto::Parametro::CAN_ID:
          ok = movimento::impostaCan((uint16_t)c.valore, movimento::canKbit());
          break;
        case proto::Parametro::CAN_KBIT:
          ok = movimento::impostaCan(movimento::canId(), (uint16_t)c.valore);
          break;
        case proto::Parametro::TIPO_ANTENNA:
          ok = (c.valore == 0 || c.valore == 1) && !movimento::inMovimento() && !targetInSospeso;
          if (ok && (c.valore == 1) != tipo_antenna::tx()) {
            tipo_antenna::salva(c.valore == 1);
            riavvioRichiesto = true;
          }
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

// --- Ricerca del canale -------------------------------------------------------------------------
// Il pannello segue il canale del router; se il router lo cambia l'antenna non sente più niente. Allora prova
// tutti i canali, partendo da quello salvato, con un messaggio di stato: la radio del pannello lo conferma (ACK)
// solo sul canale giusto. Con il pannello acceso dura meno di un secondo; il motore è fermo, quindi il loop
// può aspettare.

uint32_t ultimaRicerca = 0;

void cercaCanale() {
  if (!radio.peerValido() || associazione::fase() != associazione::Fase::INATTIVA) return;
  if (movimento::inMovimento() || targetInSospeso) return;
  uint32_t ora = millis();
  if (ora - ultimoMessaggioRadio < cfg::RICERCA_CANALE_SILENZIO_MS) return;  // 0 = dall'accensione
  if (ultimaRicerca && ora - ultimaRicerca < cfg::RICERCA_CANALE_INTERVALLO_MS) return;
  ultimaRicerca = ora;

  proto::Risposta r = preparaRisposta(0);
  uint8_t prima = canale;
  if (!ricerca_canale::cerca(radio, canale, &r, sizeof(r), cfg::RICERCA_CANALE_ACK_MS)) return;  // pannello spento
  statoUltimoInvio = r.stato;
  if (canale != prima) {
    Serial.printf("Pannello trovato sul canale %u (prima %u): salvato\n", canale, prima);
    nvs.putUChar("canale", canale);
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
    ultimoMessaggioRadio = millis();  // il pannello è appena stato sentito: niente ricerca del canale
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
  d.tx = tipo_antenna::tx();
  d.can = movimento::viaCan();
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
  Serial.printf("Antenna: %s\n", tipo_antenna::tx() ? "TX (NEMA11 + TMC2209)" : "RX (SERVO42D)");
  if (movimento::viaCan())
    Serial.printf("Motore: CAN (SERVO42D indirizzo %u, %u kbit/s)\n", movimento::canId(), movimento::canKbit());
  else
    Serial.println("Motore: STEP/DIR (TMC2209)");
}

void aiuto() {
  Serial.println(
      "Comandi: goto <kHz> | stop | zero | jog <passi> | cal <kHz> | calelim <kHz> | calcanc <kHz> |\n"
      "         tabella | stato | mac <AA:BB:CC:DD:EE:FF> | canale <1-13> | associa |\n"
      "         tipo tx|rx | canid <1-2047> | cankbit <125|250|500|1000> (solo RX) | io (finecorsa) | aiuto");
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
  } else if (!strcmp(cmd, "tipo") && arg) {
    c.tipo = proto::Tipo::IMPOSTA;
    c.freq_hz = (uint32_t)proto::Parametro::TIPO_ANTENNA;
    c.valore = !strcmp(arg, "tx") ? 1 : 0;
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
  } else if (!strcmp(cmd, "io")) {
    movimento::stampaIngressi();
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
  tipo_antenna::inizia();
  Serial.printf("\nloopant - unità antenna %s\n", tipo_antenna::tx() ? "TX (NEMA11 + TMC2209)" : "RX (SERVO42D)");

  nvs.begin("loopant", false);
  movimento::inizia();
  calibrazione::inizia(tipo_antenna::tx());
  associazione::inizia(0, tipo_antenna::tx() ? proto::FLAG_VERSIONE_TX : 0);  // pulsante BOOT

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
  cercaCanale();
  leggiConsole();
  aggiornaAssociazione();
  aggiornaOled();

  if (riavvioRichiesto) {  // nuovo tipo di antenna: la risposta è già partita
    Serial.println("Tipo di antenna cambiato: riavvio");
    delay(300);
    ESP.restart();
  }

  // invio spontaneo quando cambia lo stato
  if (radio.peerValido() && statoAttuale() != statoUltimoInvio) inviaRisposta(0);
}
