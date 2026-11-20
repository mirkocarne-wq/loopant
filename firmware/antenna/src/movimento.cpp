#include "movimento.h"

#include <Arduino.h>
#include <FastAccelStepper.h>

#include "config.h"

#if VERSIONE_TX
#include <TMCStepper.h>
static TMC2209Stepper driver(&Serial2, cfg::R_SENSE, cfg::TMC_INDIRIZZO);
#endif

namespace movimento {
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

#if VERSIONE_TX
uint32_t ultimoSg = 0;
void controllaBlocco() {
  if (cfg::SOGLIA_BLOCCO == 0 || millis() - ultimoSg < 50) return;
  ultimoSg = millis();
  if (stepper->isRampGeneratorActive() && stepper->getCurrentAcceleration() == 0 &&
      driver.SG_RESULT() < cfg::SOGLIA_BLOCCO)
    interrompi(proto::Errore::BLOCCO);
}
#endif

}  // namespace

void inizia() {
  pinMode(cfg::PIN_FC_MIN, INPUT_PULLUP);
  pinMode(cfg::PIN_FC_MAX, INPUT_PULLUP);

#if VERSIONE_TX
  Serial2.begin(115200, SERIAL_8N1, cfg::PIN_TMC_RX, cfg::PIN_TMC_TX);
  driver.begin();
  driver.pdn_disable(true);       // PDN_UART usato come UART
  driver.I_scale_analog(false);
  driver.mstep_reg_select(true);  // micropassi da registro
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(cfg::CORRENTE_MA);
  driver.microsteps(cfg::MICROPASSI);
  driver.en_spreadCycle(false);  // stealthChop
  driver.pwm_autoscale(true);
  driver.TCOOLTHRS(0xFFFFF);     // SG_RESULT valido a tutte le velocità
  uint8_t esito = driver.test_connection();
  if (esito) Serial.printf("ATTENZIONE: TMC2209 non risponde sull'UART (codice %u)\n", esito);
#endif

  motore.init();
  stepper = motore.stepperConnectToPin(cfg::PIN_STEP);
  if (!stepper) {
    Serial.println("ERRORE: impossibile inizializzare il pin STEP");
    return;
  }
  stepper->setDirectionPin(cfg::PIN_DIR);
  stepper->setEnablePin(cfg::PIN_EN, true);  // attivo basso
  stepper->setAutoEnable(false);
  stepper->setAcceleration(cfg::ACCELERAZIONE);
  stepper->disableOutputs();
}

bool vaiA(int32_t pos) {
  if (!stepper || !azzerato_ || azzeramento(fase)) return false;
  target = constrain(pos, cfg::POS_MIN, cfg::POS_MAX);
  stepper->enableOutputs();
  if (target > stepper->getCurrentPosition()) {
    avvia(target, cfg::VELOCITA_HZ);
    fase = Fase::VERSO_TARGET;
  } else {
    avvia(target - cfg::RECUPERO_GIOCO_PASSI, cfg::VELOCITA_HZ);
    fase = Fase::VERSO_RECUPERO;
  }
  return true;
}

void azzera() {
  if (!stepper) return;
  azzerato_ = false;
  stepper->enableOutputs();
  stepper->setSpeedInHz(cfg::VELOCITA_AZZERAMENTO_HZ);
  if (finecorsaMin()) {
    // già sul finecorsa: si stacca e poi si riavvicina lentamente
    stepper->setCurrentPosition(0);
    avvia(cfg::STACCO_PASSI, cfg::VELOCITA_AZZERAMENTO_HZ);
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

#if VERSIONE_TX
  if (fase == Fase::VERSO_TARGET || fase == Fase::VERSO_RECUPERO) controllaBlocco();
#endif

  switch (fase) {
    case Fase::INATTIVO:
      break;

    case Fase::VERSO_RECUPERO:
      if (fermo()) {
        avvia(target, cfg::VELOCITA_HZ);
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
      } else if (inizioAzzeramento - stepper->getCurrentPosition() > cfg::AZZERAMENTO_MAX_PASSI) {
        interrompi(proto::Errore::AZZERAMENTO_FALLITO);
      }
      break;

    case Fase::AZ_ATTESA_1:
      if (fermo()) {
        stepper->setCurrentPosition(0);
        avvia(cfg::STACCO_PASSI, cfg::VELOCITA_AZZERAMENTO_HZ);
        fase = Fase::AZ_STACCA;
      }
      break;

    case Fase::AZ_STACCA:
      if (fermo()) {
        if (finecorsaMin()) {  // non si è staccato: meccanica bloccata o finecorsa guasto
          interrompi(proto::Errore::AZZERAMENTO_FALLITO);
          break;
        }
        stepper->setSpeedInHz(cfg::VELOCITA_LENTA_HZ);
        stepper->runBackward();
        fase = Fase::AZ_LENTO;
      }
      break;

    case Fase::AZ_LENTO:
      if (finecorsaMin()) {
        stepper->forceStop();
        fase = Fase::AZ_ATTESA_2;
      } else if (stepper->getCurrentPosition() < -cfg::STACCO_PASSI) {
        interrompi(proto::Errore::AZZERAMENTO_FALLITO);
      }
      break;

    case Fase::AZ_ATTESA_2:
      if (fermo()) {
        stepper->setCurrentPosition(0);  // zero dove il motore si è fermato davvero
        avvia(cfg::STACCO_PASSI, cfg::VELOCITA_AZZERAMENTO_HZ);
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
bool inAzzeramento() { return azzeramento(fase); }
bool azzerato() { return azzerato_; }
int32_t posizione() { return stepper ? stepper->getCurrentPosition() : 0; }
bool finecorsaMin() { return attivo(cfg::PIN_FC_MIN); }
bool finecorsaMax() { return attivo(cfg::PIN_FC_MAX); }
proto::Errore errore() { return errore_; }
void cancellaErrore() { errore_ = proto::Errore::NESSUNO; }

}  // namespace movimento
