// Driver CAN per il SERVO42D (antenna RX): transceiver Waveshare SN65HVD230 su GPIO 17/16.
//
// Protocollo (da verificare sul manuale ufficiale MKS SERVO42D/57D CAN V1.0.3; formato ricavato dalla
// libreria della comunità MKSServoCAN): frame standard con identificatore = indirizzo del SERVO42D,
// primo byte = codice del comando, ultimo byte = CRC = (indirizzo + somma dei byte) & 0xFF.
//   0x82 modo di lavoro   0x8C risposte attive   0xF3 abilita/libera   0xFE posizione assoluta in impulsi
//   0x91 azzeramento con i finecorsa del SERVO42D   0x33 impulsi (posizione di 0xFE)   0xF1 stato del motore
//   0x31 encoder cumulativo (dall'accensione del driver: non è la posizione di 0xFE, solo per la console)
//   0x9E rimappatura dei finecorsa   0x90 parametri di zero e limiti   0x34 stato degli ingressi
//   0x00 lettura di un parametro (firmware V1.0.6 e successivi)
// Finecorsa (manuale CAN V1.0.9, 1.7, 1.8, 7.3, parte 8): il 42D ha un solo ingresso, IN_1. Con la rimappatura
// (9E, solo nel modo CAN) lo zero va su En e il limite CW su Dir, COM a 3,3 V; IN_1 non conta più. I limiti
// valgono solo con EndLimit attivo (0x90 o menu) e dopo un azzeramento.
// Le posizioni sono in impulsi con i micropassi impostati sul SERVO42D, come in STEP/DIR: la tabella di
// calibrazione vale per entrambi i modi.
#include <Arduino.h>
#include <driver/twai.h>
#include <string.h>

#include "config.h"
#include "motori.h"

namespace can42d {
namespace {

enum class Fase : uint8_t { INATTIVO, VERSO_RECUPERO, VERSO_TARGET, AZZERAMENTO, FERMATA };

uint16_t indirizzo = cfg::CAN_ID_PREDEFINITO;
uint16_t kbit = cfg::CAN_KBIT_PREDEFINITO;
bool installato = false;

Fase fase = Fase::INATTIVO;
bool azzerato_ = false;
proto::Errore errore_ = proto::Errore::NESSUNO;
int32_t pos_ = 0;     // posizione nota (impulsi)
int32_t target = 0;   // destinazione finale
int32_t tappa = 0;    // destinazione del movimento in corso
uint32_t inizioFase = 0;

// comando in attesa di risposta (per ripeterlo)
uint8_t ultimo[8];
uint8_t lunghezzaUltimo = 0;
bool attesa = false;
uint8_t tentativi = 0;
uint32_t istanteInvio = 0;
uint32_t ultimoStato = 0;
int32_t partenza = 0;  // posizione all'inizio del movimento in corso

// Finecorsa: configurati sul SERVO42D a ogni avvio del bus (scritti solo se diversi), riprovati all'azzeramento
bool finecorsaPronti = false;
uint8_t ingressi = 0;  // 0x34: bit 0 = En (zero), bit 1 = Dir (limite CW) con la rimappatura
bool ingressiValidi = false;
bool stampaIngressi_ = false;
bool stampaPosizione = false;
bool stampaEncoder = false;
uint8_t livelloAttivo = cfg::SERVO_HOME_LIVELLO;  // livello del finecorsa premuto (0 basso, 1 alto)
uint16_t giriZero = cfg::SERVO_HOME_RPM;            // HmSpeed letto dal SERVO42D
uint32_t limiteZeroMs = 90000;                      // oltre, azzeramento fallito (calcolato in azzera())

uint8_t crc(const uint8_t* d, uint8_t n) {
  uint32_t s = indirizzo;
  for (uint8_t i = 0; i < n; i++) s += d[i];
  return (uint8_t)(s & 0xFF);
}

bool spedisci(const uint8_t* d, uint8_t n) {
  if (!installato) return false;
  twai_message_t m = {};
  m.identifier = indirizzo;
  m.data_length_code = n + 1;
  memcpy(m.data, d, n);
  m.data[n] = crc(d, n);
  return twai_transmit(&m, pdMS_TO_TICKS(10)) == ESP_OK;
}

// Comando con risposta attesa: viene ripetuto se la risposta non arriva.
void comando(const uint8_t* d, uint8_t n) {
  memcpy(ultimo, d, n);
  lunghezzaUltimo = n;
  attesa = true;
  tentativi = 1;
  istanteInvio = millis();
  spedisci(d, n);
}

void abilita(bool si) {
  const uint8_t d[] = {0xF3, (uint8_t)(si ? 1 : 0)};
  spedisci(d, sizeof(d));
}

uint16_t rpm() {  // velocità in giri/min del motore, dagli impulsi al secondo di config.h
  uint32_t r = (uint32_t)cfg::RX.velocitaHz * 60UL / (200UL * cfg::RX.micropassi);
  return (uint16_t)(r > 3000 ? 3000 : (r < 1 ? 1 : r));
}

void muoviA(int32_t pos) {
  // Mai sotto lo zero (dove il SERVO42D ha finito l'azzeramento): ultima difesa, qualunque cosa chiedano
  // gli strati sopra. L'azzeramento non passa da qui (comando 0x91).
  if (pos < 0) pos = 0;
  partenza = pos_;
  tappa = pos;
  uint16_t v = rpm();
  const uint8_t d[] = {0xFE,
                       (uint8_t)(v >> 8),
                       (uint8_t)v,
                       cfg::SERVO_ACCELERAZIONE,
                       (uint8_t)((uint32_t)pos >> 16),
                       (uint8_t)((uint32_t)pos >> 8),
                       (uint8_t)pos};
  comando(d, sizeof(d));
}

// Posizione dopo uno stop: impulsi contati dallo zero (0x33), le stesse unità di 0xFE (manuale 11.2.3).
// L'encoder 0x31 conta dall'accensione del SERVO42D: il 25/11/2026 dava 31 919 con 0xFE limitato a 31 645.
void leggiPosizione() {
  const uint8_t d[] = {0x33};
  spedisci(d, sizeof(d));
}

void arrestaAzzeramento() {
  const uint8_t d[] = {0xF7};  // arresto d'emergenza (lo zero va piano: sotto i 1000 giri/min del manuale)
  spedisci(d, sizeof(d));
}

void leggiIngressi() {
  const uint8_t d[] = {0x34};
  spedisci(d, sizeof(d));
}

// Premuto = bit uguale a HmTrig: da verificare con il comando "io" della console.
bool premuto(uint8_t bit) { return ingressiValidi && (((ingressi >> bit) & 1) == livelloAttivo); }

void termina() {
  fase = Fase::INATTIVO;
  attesa = false;
  abilita(false);  // motore libero dopo ogni movimento
  leggiIngressi();  // i finecorsa cambiano solo muovendo: l'ultima lettura resta valida a motore fermo
}

void errore(proto::Errore e) {
  errore_ = e;
  if (e != proto::Errore::NESSUNO && e != proto::Errore::COMANDO_NON_VALIDO) azzerato_ = false;
  termina();
}

void movimentoFinito() {
  pos_ = tappa;
  if (fase == Fase::VERSO_RECUPERO) {
    fase = Fase::VERSO_TARGET;
    muoviA(target);
  } else {
    termina();
  }
}

void ricevi(const twai_message_t& m) {
  if (m.identifier != indirizzo || m.data_length_code < 2) return;
  uint8_t n = m.data_length_code;
  if (m.data[n - 1] != crc(m.data, n - 1)) return;  // CRC errato
  uint8_t codice = m.data[0], stato = m.data[1];
  if (attesa && codice == ultimo[0]) attesa = false;

  switch (codice) {
    case 0xFE:  // 0 errore, 1 partito, 2 arrivato, 3 fermato da un finecorsa
      if (stato == 0) errore(proto::Errore::MOTORE_CAN);
      else if (stato == 2) {
        if (fase == Fase::FERMATA) {
          termina();
          leggiPosizione();
        } else if (fase == Fase::VERSO_RECUPERO || fase == Fase::VERSO_TARGET) {
          movimentoFinito();
        }
      } else if (stato == 3) {
        if (fase == Fase::VERSO_TARGET && tappa < partenza && tappa < cfg::RX.staccoPassi) {
          // Finecorsa dello zero toccato scendendo verso lo 0 con il jog libero: l'interruttore scatta un soffio
          // sopra il punto dove l'azzeramento si è fermato. Non è un errore: la posizione vale 0, come dopo lo
          // zero (così girare ancora indietro non manda niente).
          pos_ = 0;
          termina();
        } else {
          errore(proto::Errore::FINECORSA);
          leggiPosizione();
        }
      }
      break;
    case 0x91:  // 0 errore, 1 partito, 2 riuscito
      if (fase != Fase::AZZERAMENTO) break;
      if (stato == 0) errore(proto::Errore::AZZERAMENTO_FALLITO);
      else if (stato == 2) {  // zero dove il SERVO42D ha finito l'azzeramento: si resta lì fino al primo TUNE
        pos_ = 0;
        azzerato_ = true;
        termina();
      }
      break;
    case 0x33:  // impulsi dallo zero, int32
      if (n >= 6) {
        int32_t v = (int32_t)((uint32_t)m.data[1] << 24 | (uint32_t)m.data[2] << 16 | (uint32_t)m.data[3] << 8 |
                              m.data[4]);
        if (fase == Fase::INATTIVO || fase == Fase::FERMATA) pos_ = v;
        if (stampaPosizione) {
          stampaPosizione = false;
          Serial.printf("Posizione SERVO42D: %ld impulsi dallo zero (0x33)\n", (long)v);
        }
      }
      break;
    case 0x31:  // encoder cumulativo a 48 bit, 0x4000 per giro del motore: solo per la console
      if (n >= 8 && stampaEncoder) {
        stampaEncoder = false;
        int64_t v = 0;
        for (int i = 1; i <= 6; i++) v = (v << 8) | m.data[i];
        if (v & ((int64_t)1 << 47)) v |= ~(((int64_t)1 << 48) - 1);
        Serial.printf("Encoder SERVO42D: %ld impulsi dall'accensione del driver (0x31)\n",
                      (long)(v * (200LL * cfg::RX.micropassi) / 0x4000));
      }
      break;
    case 0x34:  // stato degli ingressi
      ingressi = stato;
      ingressiValidi = true;
      if (stampaIngressi_) {
        stampaIngressi_ = false;
        Serial.printf("Ingressi SERVO42D: 0x%02X · En (zero) %u · Dir (limite CW) %u · premuto = %u (HmTrig %s)"
                      " -> zero %s, limite CW %s\n",
                      ingressi, (unsigned)(ingressi & 1), (unsigned)((ingressi >> 1) & 1), (unsigned)livelloAttivo,
                      livelloAttivo ? "alto" : "basso", premuto(0) ? "PREMUTO" : "libero",
                      premuto(1) ? "PREMUTO" : "libero");
      }
      break;
    case 0xF1:  // 1 = fermo: rete di sicurezza se la risposta "arrivato" si è persa
      if (stato == 1 && millis() - inizioFase > 500) {
        if (fase == Fase::VERSO_RECUPERO || fase == Fase::VERSO_TARGET) movimentoFinito();
        else if (fase == Fase::FERMATA) {
          termina();
          leggiPosizione();
        }
      }
      break;
    default:
      break;
  }
}


// --- Configurazione dei finecorsa sul SERVO42D -------------------------------------------------------------
// Risposte attese qui, a motore fermo (avvio del bus o prima dell'azzeramento); le altre vanno a ricevi().

bool risposta(uint8_t codice, twai_message_t& r, uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    twai_message_t m;
    if (twai_receive(&m, pdMS_TO_TICKS(5)) != ESP_OK) continue;
    uint8_t n = m.data_length_code;
    if (m.identifier == indirizzo && n >= 2 && m.data[0] == codice && m.data[n - 1] == crc(m.data, n - 1)) {
      r = m;
      return true;
    }
    ricevi(m);
  }
  return false;
}

bool chiedi(const uint8_t* d, uint8_t n, uint8_t codice, twai_message_t& r) {
  for (int i = 0; i < 2; i++) {
    if (spedisci(d, n) && risposta(codice, r, 100)) return true;
  }
  return false;
}

// Stato dei finecorsa sul SERVO42D, letto con 0x00 (firmware V1.0.6 e successivi)
struct Limiti {
  bool letto = false;
  uint8_t mappa = 0;  // 9E: 1 = zero su En, limite CW su Dir
  uint8_t livello = 0, verso = 0, endLimit = 0, modo = 0;
  uint16_t giri = 0;
};

Limiti leggiLimiti() {
  Limiti l;
  twai_message_t r;
  const uint8_t m[] = {0x00, 0x9E};
  if (!chiedi(m, sizeof(m), 0x9E, r) || r.data_length_code != 3) return l;  // 4 byte (FF FF) = non leggibile
  l.mappa = r.data[1];
  const uint8_t z[] = {0x00, 0x90};
  if (!chiedi(z, sizeof(z), 0x90, r) || r.data_length_code < 7) return l;
  l.livello = r.data[1];  // 90 livello verso velocità(2) EndLimit [modo] CRC
  l.verso = r.data[2];
  l.giri = (uint16_t)(r.data[3] << 8 | r.data[4]);
  l.endLimit = r.data[5];
  l.modo = r.data_length_code >= 8 ? r.data[6] : 0;
  l.letto = true;
  return l;
}

bool limitiGiusti(const Limiti& l) {
  return l.letto && l.mappa == 1 && l.livello == cfg::SERVO_HOME_LIVELLO && l.endLimit == 1 && l.modo == 0;
}

// Rimappatura (9E): zero su En, limite CW su Dir. Poi 0x90 con EndLimit attivo e il livello di config.h; verso e
// velocità dello zero restano quelli del menu del SERVO42D (HmDir, HmSpeed). Si scrive solo se serve, per non
// consumare la flash del driver a ogni accensione, e si rilegge per verificare: il 25/11/2026 il SERVO42D V1.0.9
// ha risposto "non riuscito" (stato 0) a tutte le scritture per qualche minuto, poi la 0x90 è passata (forse con il
// suo menu aperto: da verificare). Il manuale chiede un azzeramento dopo ogni modifica: nessun movimento prima
// dello ZERO.
bool configuraFinecorsa() {
  Limiti l = leggiLimiti();
  if (!limitiGiusti(l)) {
    twai_message_t r;
    if (!l.letto || l.mappa != 1) {
      const uint8_t mappa[] = {0x9E, 1};
      chiedi(mappa, sizeof(mappa), 0x9E, r);
    }
    uint8_t verso = l.letto ? l.verso : cfg::SERVO_HOME_VERSO;
    uint16_t giri = l.letto ? l.giri : cfg::SERVO_HOME_RPM;
    const uint8_t zero[] = {0x90, cfg::SERVO_HOME_LIVELLO, verso, (uint8_t)(giri >> 8), (uint8_t)giri, 1, 0};
    chiedi(zero, sizeof(zero), 0x90, r);
    l = leggiLimiti();
  }
  livelloAttivo = cfg::SERVO_HOME_LIVELLO;
  if (l.letto && l.giri > 0) giriZero = l.giri;
  if (!l.letto) {
    Serial.println("SERVO42D: parametri dei finecorsa non letti (driver spento o firmware prima di V1.0.6)");
    return false;
  }
  if (!limitiGiusti(l)) {
    Serial.printf("SERVO42D: finecorsa da sistemare nel menu del driver (scrittura via CAN rifiutata):%s%s%s%s\n",
                  l.mappa != 1 ? " rimappatura spenta (solo da CAN, 9E)," : "",
                  l.livello != cfg::SERVO_HOME_LIVELLO ? (cfg::SERVO_HOME_LIVELLO ? " HmTrig High," : " HmTrig Low,") : "",
                  l.endLimit != 1 ? " EndLimit Enable," : "", l.modo != 0 ? " HmMode con interruttore," : "");
    return false;
  }
  Serial.printf("SERVO42D: finecorsa pronti (zero %s su En, limite su Dir, livello %s, %u giri/min)\n",
                l.verso ? "CCW" : "CW", l.livello ? "alto" : "basso", l.giri);
  return true;
}

}  // namespace


void configura(uint16_t id, uint16_t k) {
  indirizzo = id;
  kbit = k;
}

void inizia() {
  if (installato) return;
  twai_general_config_t g =
      TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)cfg::PIN_CAN_TX, (gpio_num_t)cfg::PIN_CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t;
  switch (kbit) {
    case 125: t = TWAI_TIMING_CONFIG_125KBITS(); break;
    case 250: t = TWAI_TIMING_CONFIG_250KBITS(); break;
    case 1000: t = TWAI_TIMING_CONFIG_1MBITS(); break;
    default: t = TWAI_TIMING_CONFIG_500KBITS(); break;
  }
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) {
    Serial.println("ERRORE: bus CAN non avviato");
    errore_ = proto::Errore::MOTORE_CAN;
    return;
  }
  installato = true;
  const uint8_t modo[] = {0x82, cfg::SERVO_MODO_CAN};
  spedisci(modo, sizeof(modo));
  const uint8_t risposte[] = {0x8C, 1, 1};  // risposte e avvisi di fine movimento attivi
  spedisci(risposte, sizeof(risposte));
  abilita(false);
  fase = Fase::INATTIVO;
  azzerato_ = false;
  // Subito, così 0x34 legge già En e Dir; se il SERVO42D non risponde (spento) si riprova al primo azzeramento.
  // Nessun movimento prima dello zero, che il manuale chiede dopo ogni modifica dei limiti.
  finecorsaPronti = configuraFinecorsa();
  leggiIngressi();
}

void fine() {
  if (!installato) return;
  const uint8_t modo[] = {0x82, cfg::SERVO_MODO_IMPULSI};  // il SERVO42D torna a STEP/DIR
  spedisci(modo, sizeof(modo));
  delay(20);
  twai_stop();
  twai_driver_uninstall();
  installato = false;
  fase = Fase::INATTIVO;
  azzerato_ = false;
}

bool vaiA(int32_t pos, bool recupero) {
  if (!installato || !azzerato_ || fase == Fase::AZZERAMENTO) return false;
  // con il recupero del gioco serve spazio sotto il target (posMin); il jog libero può scendere fino allo zero
  target = constrain(pos, recupero ? cfg::RX.posMin() : 0, cfg::RX.posMax());
  if (!recupero && target == pos_) return true;  // già lì (per esempio a 0 girando ancora indietro): niente impulsi
  abilita(true);
  inizioFase = millis();
  if (target > pos_ || !recupero) {
    fase = Fase::VERSO_TARGET;
    muoviA(target);
  } else {  // si arriva sempre dal verso crescente: prima sotto il target, poi su
    fase = Fase::VERSO_RECUPERO;
    muoviA(target - cfg::RX.recuperoGiocoPassi);
  }
  return true;
}

void azzera() {
  if (!installato) {
    errore_ = proto::Errore::MOTORE_CAN;
    return;
  }
  azzerato_ = false;
  if (!finecorsaPronti) finecorsaPronti = configuraFinecorsa();
  if (!finecorsaPronti) {  // con rimappatura o livello sbagliati lo zero non si troverebbe: meglio non partire
    errore_ = proto::Errore::MOTORE_CAN;
    return;
  }
  // Tempo per la corsa intera alla velocità di zero, più metà e 3 s: se il finecorsa non arriva (filo staccato)
  // il motore non gira a vuoto per minuti. 60 giri/min: circa 20 s.
  uint32_t passiAlSecondo = (uint32_t)giriZero * cfg::RX_PASSI_MOTORE / 60;
  limiteZeroMs = 3000 + (uint32_t)((uint64_t)cfg::RX.azzeramentoMaxPassi() * 1500 / passiAlSecondo);
  abilita(true);
  fase = Fase::AZZERAMENTO;
  inizioFase = millis();
  const uint8_t d[] = {0x91};  // parametri di azzeramento dal menu del SERVO42D
  comando(d, sizeof(d));
}

void ferma() {
  if (fase == Fase::INATTIVO) return;
  if (fase == Fase::AZZERAMENTO) {  // l'azzeramento non è un movimento in posizione: arresto d'emergenza
    azzerato_ = false;
    arrestaAzzeramento();
    termina();
    return;
  }
  fase = Fase::FERMATA;
  inizioFase = millis();
  const uint8_t d[] = {0xFE, 0, 0, cfg::SERVO_ACCELERAZIONE, 0, 0, 0};  // velocità 0: arresto con rampa
  comando(d, sizeof(d));
}

void aggiorna() {
  if (!installato) return;
  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) ricevi(m);

  uint32_t ora = millis();
  if (attesa && ora - istanteInvio > cfg::SERVO_TIMEOUT_MS) {
    if (tentativi < 3) {
      tentativi++;
      istanteInvio = ora;
      spedisci(ultimo, lunghezzaUltimo);
    } else {
      attesa = false;
      errore(proto::Errore::MOTORE_CAN);
    }
  }
  if (fase != Fase::INATTIVO) {
    if (ora - ultimoStato > 300) {  // stato del motore, per non dipendere solo dall'avviso di fine
      ultimoStato = ora;
      const uint8_t d[] = {0xF1};
      spedisci(d, sizeof(d));
      leggiIngressi();  // finecorsa per la Diagnostica del pannello (a motore fermo niente traffico sul bus)
    }
    if (fase == Fase::AZZERAMENTO && ora - inizioFase > limiteZeroMs) {
      Serial.printf("SERVO42D: finecorsa dello zero non trovato in %lu s\n", (unsigned long)(limiteZeroMs / 1000));
      arrestaAzzeramento();
      errore(proto::Errore::AZZERAMENTO_FALLITO);
    } else if (fase != Fase::AZZERAMENTO && ora - inizioFase > 90000) {
      errore(proto::Errore::MOTORE_CAN);
    }
  }
}

bool inMovimento() { return fase != Fase::INATTIVO; }
bool inAzzeramento() { return fase == Fase::AZZERAMENTO; }
bool azzerato() { return azzerato_; }
int32_t posizione() { return pos_; }
bool finecorsaMin() { return premuto(0); }  // sul SERVO42D: letti con 0x34
bool finecorsaMax() { return premuto(1); }
void stampaIngressi() {
  if (!installato) {
    Serial.println("Bus CAN non avviato");
    return;
  }
  stampaIngressi_ = stampaPosizione = stampaEncoder = true;
  leggiIngressi();
  leggiPosizione();
  const uint8_t d[] = {0x31};
  spedisci(d, sizeof(d));
}
bool driverAbilitato() { return fase != Fase::INATTIVO; }
proto::Errore errore() { return errore_; }
void cancellaErrore() { errore_ = proto::Errore::NESSUNO; }

}  // namespace can42d
