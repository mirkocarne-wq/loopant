// Driver STEP/DIR/EN: TMC2209 (TX) oppure SERVO42D in modo a impulsi (RX).
// Finecorsa sui GPIO dell'ESP32, azzeramento fatto dal firmware.
#include "motori.h"

#include <Arduino.h>
#include <FastAccelStepper.h>

#include <TMCStepper.h>

#include "config.h"
#include "tipo_antenna.h"

static TMC2209Stepper driver(&Serial2, cfg::R_SENSE, cfg::TMC_INDIRIZZO);  // solo antenna TX

namespace stepdir {
namespace {

enum class Fase : uint8_t {
  INATTIVO,
  VERSO_RECUPERO,  // sotto il target, per arrivare dal verso crescente
  VERSO_TARGET,
  FERMATA,         // decelerazione dopo uno stop
  AZ_CERCA,        // azzeramento: ricerca veloce del finecorsa
  AZ_ATTESA_1,
  AZ_STACCA,
  AZ_LENTO,        // riavvicinamento lento
  AZ_ATTESA_2,
  AZ_FINE,         // stacco finale
};

FastAccelStepperEngine motore;
FastAccelStepper* stepper = nullptr;
Fase fase = Fase::INATTIVO;
bool azzerato_ = false;
proto::Errore errore_ = proto::Errore::NESSUNO;
int32_t target = 0;
int32_t inizioAzzeramento = 0;

const cfg::Meccanica& M() { return tipo_antenna::meccanica(); }

bool attivo(int pin) { return digitalRead(pin) == (cfg::FC_ATTIVO_BASSO ? LOW : HIGH); }

void avvia(int32_t pos, uint32_t velocita) {
  stepper->setSpeedInHz(velocita);
  stepper->moveTo(pos);
}

void fine() {
  fase = Fase::INATTIVO;
  stepper->disableOutputs();  // motore libero dopo ogni movimento
}

void interrompi(proto::Errore e) {
  stepper->forceStop();
  errore_ = e;
  if (e == proto::Errore::FINECORSA || e == proto::Errore::AZZERAMENTO_FALLITO ||
      e == proto::Errore::BLOCCO)
    azzerato_ = false;  // la posizione non è più affidabile
  fase = Fase::FERMATA;
}

bool fermo() { return !stepper->isRunning(); }

bool azzeramento(Fase f) { return f >= Fase::AZ_CERCA; }

uint32_t ultimoSg = 0;
void controllaBlocco() {
  if (cfg::SOGLIA_BLOCCO == 0 || millis() - ultimoSg < 50) return;
  ultimoSg = millis();
  if (stepper->isRampGeneratorActive() && stepper->getCurrentAcceleration() == 0 &&
      driver.SG_RESULT() < cfg::SOGLIA_BLOCCO)
    interrompi(proto::Errore::BLOCCO);
}

}  // namespace

void inizia() {
  pinMode(cfg::PIN_FC_MIN, INPUT_PULLUP);
  pinMode(cfg::PIN_FC_MAX, INPUT_PULLUP);

  if (tipo_antenna::tx()) {  // TMC2209 configurato via UART
    Serial2.begin(115200, SERIAL_8N1, cfg::PIN_TMC_RX, cfg::PIN_TMC_TX);
    driver.begin();
    driver.pdn_disable(true);       // PDN_UART usato come UART
    driver.I_scale_analog(false);
    driver.mstep_reg_select(true);  // micropassi da registro
    driver.toff(4);
    driver.blank_time(24);
    driver.rms_current(cfg::CORRENTE_MA);
    driver.microsteps(M().micropassi);
    driver.en_spreadCycle(false);  // stealthChop
    driver.pwm_autoscale(true);
    driver.TCOOLTHRS(0xFFFFF);     // SG_RESULT valido a tutte le velocità
    uint8_t esito = driver.test_connection();
    if (esito) Serial.printf("ATTENZIONE: TMC2209 non risponde sull'UART (codice %u)\n", esito);
  }

  motore.init();
  stepper = motore.stepperConnectToPin(cfg::PIN_STEP);
  if (!stepper) {
    Serial.println("ERRORE: impossibile inizializzare il pin STEP");
    return;
  }
  stepper->setDirectionPin(cfg::PIN_DIR);
  stepper->setEnablePin(cfg::PIN_EN, true);  // attivo basso
  stepper->setAutoEnable(false);
  stepper->setAcceleration(M().accelerazione);
  stepper->disableOutputs();
}

bool vaiA(int32_t pos) {
  if (!stepper || !azzerato_ || azzeramento(fase)) return false;
  target = constrain(pos, M().posMin(), M().posMax());
  stepper->enableOutputs();
  if (target > stepper->getCurrentPosition()) {
    avvia(target, M().velocitaHz);
    fase = Fase::VERSO_TARGET;
  } else {
    avvia(target - M().recuperoGiocoPassi, M().velocitaHz);
    fase = Fase::VERSO_RECUPERO;
  }
  return true;
}

void azzera() {
  if (!stepper) return;
  azzerato_ = false;
  stepper->enableOutputs();
  stepper->setSpeedInHz(M().velocitaAzzeramentoHz);
  if (finecorsaMin()) {
    // già sul finecorsa: si stacca e poi si riavvicina lentamente
    stepper->setCurrentPosition(0);
    avvia(M().staccoPassi, M().velocitaAzzeramentoHz);
    fase = Fase::AZ_STACCA;
    return;
  }
  inizioAzzeramento = stepper->getCurrentPosition();
  stepper->runBackward();
  fase = Fase::AZ_CERCA;
}

void ferma() {
  if (!stepper || fase == Fase::INATTIVO) return;
  if (azzeramento(fase)) azzerato_ = false;
  stepper->stopMove();
  fase = Fase::FERMATA;
}

void aggiorna() {
  if (!stepper) return;

  // Finecorsa inattesi
  if (!azzeramento(fase) && fase != Fase::FERMATA && fase != Fase::INATTIVO) {
    if (finecorsaMin() || finecorsaMax()) {
      interrompi(proto::Errore::FINECORSA);
      return;
    }
  }
  if (azzeramento(fase) && finecorsaMax()) {
    interrompi(proto::Errore::FINECORSA);
    return;
  }

  if (tipo_antenna::tx() && (fase == Fase::VERSO_TARGET || fase == Fase::VERSO_RECUPERO)) controllaBlocco();

  switch (fase) {
    case Fase::INATTIVO:
      break;

    case Fase::VERSO_RECUPERO:
      if (fermo()) {
        avvia(target, M().velocitaHz);
        fase = Fase::VERSO_TARGET;
      }
      break;

    case Fase::VERSO_TARGET:
    case Fase::FERMATA:
      if (fermo()) fine();
      break;

    case Fase::AZ_CERCA:
      if (finecorsaMin()) {
        stepper->forceStop();
        fase = Fase::AZ_ATTESA_1;
      } else if (inizioAzzeramento - stepper->getCurrentPosition() > M().azzeramentoMaxPassi()) {
        interrompi(proto::Errore::AZZERAMENTO_FALLITO);
      }
      break;

    case Fase::AZ_ATTESA_1:
      if (fermo()) {
        stepper->setCurrentPosition(0);
        avvia(M().staccoPassi, M().velocitaAzzeramentoHz);
        fase = Fase::AZ_STACCA;
      }
      break;

    case Fase::AZ_STACCA:
      if (fermo()) {
        if (finecorsaMin()) {  // non si è staccato: meccanica bloccata o finecorsa guasto
          interrompi(proto::Errore::AZZERAMENTO_FALLITO);
          break;
        }
        stepper->setSpeedInHz(M().velocitaLentaHz);
        stepper->runBackward();
        fase = Fase::AZ_LENTO;
      }
      break;

    case Fase::AZ_LENTO:
      if (finecorsaMin()) {
        stepper->forceStop();
        fase = Fase::AZ_ATTESA_2;
      } else if (stepper->getCurrentPosition() < -M().staccoPassi) {
        interrompi(proto::Errore::AZZERAMENTO_FALLITO);
      }
      break;

    case Fase::AZ_ATTESA_2:
      if (fermo()) {
        stepper->setCurrentPosition(0);  // zero dove il motore si è fermato davvero
        avvia(M().staccoPassi, M().velocitaAzzeramentoHz);
        fase = Fase::AZ_FINE;
      }
      break;

    case Fase::AZ_FINE:
      if (fermo()) {
        azzerato_ = true;
        fine();
      }
      break;
  }
}

bool inMovimento() { return fase != Fase::INATTIVO; }
bool driverAbilitato() { return fase != Fase::INATTIVO; }
bool inAzzeramento() { return azzeramento(fase); }
bool azzerato() { return azzerato_; }
int32_t posizione() { return stepper ? stepper->getCurrentPosition() : 0; }
bool finecorsaMin() { return attivo(cfg::PIN_FC_MIN); }
bool finecorsaMax() { return attivo(cfg::PIN_FC_MAX); }
proto::Errore errore() { return errore_; }
void cancellaErrore() { errore_ = proto::Errore::NESSUNO; }

}  // namespace stepdir
