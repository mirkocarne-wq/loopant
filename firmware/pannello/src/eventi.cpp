#include "eventi.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace eventi {
namespace {

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Evento registro[MAX_EVENTI];
uint8_t quanti = 0, testa = 0;
Contatori cont = {0, 0, 0, "--"};

bool primo = true;
bool eraCollegato = false;
proto::Stato statoPrima = proto::Stato::NON_AZZERATA;

void aggiungi(Tipo tipo, const char* testo) {
  Evento e;
  oraTesto(e.ora, sizeof(e.ora));
  e.tipo = tipo;
  strncpy(e.testo, testo, sizeof(e.testo) - 1);
  e.testo[sizeof(e.testo) - 1] = 0;
  portENTER_CRITICAL(&mux);
  registro[testa] = e;
  testa = (testa + 1) % MAX_EVENTI;
  if (quanti < MAX_EVENTI) quanti++;
  portEXIT_CRITICAL(&mux);
}

}  // namespace

void oraTesto(char* out, unsigned n) {
  time_t adesso = time(nullptr);
  if (adesso > 1700000000) {
    struct tm utc;
    gmtime_r(&adesso, &utc);
    snprintf(out, n, "%02d:%02d", utc.tm_hour, utc.tm_min);
  } else {
    uint32_t s = millis() / 1000;
    snprintf(out, n, "+%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60));
  }
}

void osserva(const link_antenna::Stato& s) {
  char buf[56];
  if (primo) {
    primo = false;
    eraCollegato = s.collegato;
    statoPrima = s.r.stato;
    return;
  }
  if (s.collegato != eraCollegato) {
    eraCollegato = s.collegato;
    aggiungi(Tipo::COLLEGAMENTO, s.collegato ? "Collegamento con l'antenna attivo" : "Collegamento con l'antenna perso");
  }
  if (!s.collegato || s.r.stato == statoPrima) return;

  proto::Stato prima = statoPrima;
  statoPrima = s.r.stato;
  switch (s.r.stato) {
    case proto::Stato::IN_MOVIMENTO:
      portENTER_CRITICAL(&mux);
      cont.movimenti++;
      portEXIT_CRITICAL(&mux);
      break;
    case proto::Stato::AZZERAMENTO:
      portENTER_CRITICAL(&mux);
      cont.azzeramenti++;
      oraTesto(cont.ultimoZero, sizeof(cont.ultimoZero));
      portEXIT_CRITICAL(&mux);
      aggiungi(Tipo::ZERO, "Azzeramento sul finecorsa");
      break;
    case proto::Stato::FERMO:
      if (prima == proto::Stato::IN_MOVIMENTO || prima == proto::Stato::AZZERAMENTO) {
        if (s.r.freq_hz)
          snprintf(buf, sizeof(buf), "%.1f kHz · posizione %ld", s.r.freq_hz / 1000.0, (long)s.r.posizione);
        else
          snprintf(buf, sizeof(buf), "Posizione %ld", (long)s.r.posizione);
        aggiungi(Tipo::ACCORDATA, buf);
      }
      break;
    case proto::Stato::ERRORE:
      portENTER_CRITICAL(&mux);
      cont.errori++;
      portEXIT_CRITICAL(&mux);
      snprintf(buf, sizeof(buf), "Errore: %s", proto::nomeErrore(s.r.errore));
      aggiungi(Tipo::ERRORE, buf);
      break;
    case proto::Stato::NON_AZZERATA:
      break;
  }
}

uint8_t elenco(Evento* out, uint8_t max) {
  portENTER_CRITICAL(&mux);
  uint8_t n = quanti < max ? quanti : max;
  for (uint8_t i = 0; i < n; i++) out[i] = registro[(testa + MAX_EVENTI - 1 - i) % MAX_EVENTI];
  portEXIT_CRITICAL(&mux);
  return n;
}

Contatori contatori() {
  portENTER_CRITICAL(&mux);
  Contatori c = cont;
  portEXIT_CRITICAL(&mux);
  return c;
}

}  // namespace eventi
