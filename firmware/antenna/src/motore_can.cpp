// Driver CAN per il SERVO42D (antenna RX): transceiver Waveshare SN65HVD230 su GPIO 17/16.
//
// Protocollo (da verificare sul manuale ufficiale MKS SERVO42D/57D CAN V1.0.3; formato ricavato dalla
// libreria della comunità MKSServoCAN): frame standard con identificatore = indirizzo del SERVO42D,
// primo byte = codice del comando, ultimo byte = CRC = (indirizzo + somma dei byte) & 0xFF.
//   0x82 modo di lavoro   0x8C risposte attive   0xF3 abilita/libera   0xFE posizione assoluta in impulsi
//   0x91 azzeramento con i finecorsa del SERVO42D   0x31 encoder cumulativo   0xF1 stato del motore
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

void leggiEncoder() {
  const uint8_t d[] = {0x31};
  spedisci(d, sizeof(d));
}

void termina() {
  fase = Fase::INATTIVO;
  attesa = false;
  abilita(false);  // motore libero dopo ogni movimento
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
          leggiEncoder();
        } else if (fase == Fase::VERSO_RECUPERO || fase == Fase::VERSO_TARGET) {
          movimentoFinito();
        }
      } else if (stato == 3) {
        errore(proto::Errore::FINECORSA);
        leggiEncoder();
      }
      break;
    case 0x91:  // 0 errore, 1 partito, 2 riuscito
      if (fase != Fase::AZZERAMENTO) break;
      if (stato == 0) errore(proto::Errore::AZZERAMENTO_FALLITO);
      else if (stato == 2) {
        pos_ = 0;
        azzerato_ = true;
        termina();
      }
      break;
    case 0x31:  // encoder cumulativo a 48 bit, 0x4000 per giro del motore
      if (n >= 8) {
        int64_t v = 0;
        for (int i = 1; i <= 6; i++) v = (v << 8) | m.data[i];
        if (v & ((int64_t)1 << 47)) v |= ~(((int64_t)1 << 48) - 1);
        pos_ = (int32_t)(v * (200LL * cfg::RX.micropassi) / 0x4000);
      }
      break;
    case 0xF1:  // 1 = fermo: rete di sicurezza se la risposta "arrivato" si è persa
      if (stato == 1 && millis() - inizioFase > 500) {
        if (fase == Fase::VERSO_RECUPERO || fase == Fase::VERSO_TARGET) movimentoFinito();
        else if (fase == Fase::FERMATA) {
          termina();
          leggiEncoder();
        }
      }
      break;
    default:
      break;
  }
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

bool vaiA(int32_t pos) {
  if (!installato || !azzerato_ || fase == Fase::AZZERAMENTO) return false;
  target = constrain(pos, cfg::RX.posMin(), cfg::RX.posMax());
  abilita(true);
  inizioFase = millis();
  if (target > pos_) {
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
  abilita(true);
  fase = Fase::AZZERAMENTO;
  inizioFase = millis();
  const uint8_t d[] = {0x91};  // parametri di azzeramento dal menu del SERVO42D
  comando(d, sizeof(d));
}

void ferma() {
  if (fase == Fase::INATTIVO) return;
  if (fase == Fase::AZZERAMENTO) azzerato_ = false;
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
    }
    if (ora - inizioFase > 90000) errore(fase == Fase::AZZERAMENTO ? proto::Errore::AZZERAMENTO_FALLITO
                                                                   : proto::Errore::MOTORE_CAN);
  }
}

bool inMovimento() { return fase != Fase::INATTIVO; }
bool inAzzeramento() { return fase == Fase::AZZERAMENTO; }
bool azzerato() { return azzerato_; }
int32_t posizione() { return pos_; }
bool finecorsaMin() { return false; }  // collegati al SERVO42D: stato non letto
bool finecorsaMax() { return false; }
bool driverAbilitato() { return fase != Fase::INATTIVO; }
proto::Errore errore() { return errore_; }
void cancellaErrore() { errore_ = proto::Errore::NESSUNO; }

}  // namespace can42d
