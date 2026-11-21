// Posizione del sole, terminatore (grey line), alba e tramonto.
// Formule e precisione: docs/software/09-propagazione.md (qualche minuto, da verificare).
#pragma once
#include <time.h>

namespace sole {

struct Posizione {
  double declinazione;  // gradi
  double lonSubsolare;  // gradi, -180..180
  double eot;           // equazione del tempo, minuti
};

Posizione calcola(time_t t);

// Latitudine del terminatore alla longitudine data (gradi).
double latTerminatore(const Posizione& p, double lon);

// true se il punto è di notte (sole sotto l'orizzonte geometrico).
bool notte(const Posizione& p, double lat, double lon);

struct AlbaTramonto {
  bool sempreGiorno;
  bool sempreNotte;
  double alba;      // ore UTC, 0..24
  double tramonto;  // ore UTC, 0..24
};

AlbaTramonto albaTramonto(time_t t, double lat, double lon);

// Ore UTC (con decimali) dell'istante t.
double oreUtc(time_t t);

}  // namespace sole
