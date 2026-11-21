#include "sole.h"

#include <math.h>

namespace sole {
namespace {

constexpr double RAD = M_PI / 180.0;

int giornoAnno(time_t t, double& ore) {
  struct tm u;
  gmtime_r(&t, &u);
  ore = u.tm_hour + u.tm_min / 60.0 + u.tm_sec / 3600.0;
  return u.tm_yday + 1;
}

double normalizza(double lon) {
  while (lon > 180) lon -= 360;
  while (lon < -180) lon += 360;
  return lon;
}

double modulo24(double h) {
  while (h < 0) h += 24;
  while (h >= 24) h -= 24;
  return h;
}

}  // namespace

double oreUtc(time_t t) {
  double ore;
  giornoAnno(t, ore);
  return ore;
}

Posizione calcola(time_t t) {
  double ore;
  int n = giornoAnno(t, ore);
  Posizione p;
  p.declinazione = -23.44 * cos(2 * M_PI / 365.0 * (n + 10));
  double b = 2 * M_PI * (n - 81) / 365.0;
  p.eot = 9.87 * sin(2 * b) - 7.53 * cos(b) - 1.5 * sin(b);
  p.lonSubsolare = normalizza(-15.0 * (ore - 12.0 + p.eot / 60.0));
  return p;
}

double latTerminatore(const Posizione& p, double lon) {
  double d = p.declinazione;
  if (fabs(d) < 0.01) d = d < 0 ? -0.01 : 0.01;  // agli equinozi
  return atan(-cos((lon - p.lonSubsolare) * RAD) / tan(d * RAD)) / RAD;
}

bool notte(const Posizione& p, double lat, double lon) {
  double s = sin(lat * RAD) * sin(p.declinazione * RAD) +
             cos(lat * RAD) * cos(p.declinazione * RAD) * cos((lon - p.lonSubsolare) * RAD);
  return s < 0;
}

AlbaTramonto albaTramonto(time_t t, double lat, double lon) {
  Posizione p = calcola(t);
  AlbaTramonto r = {false, false, 0, 0};
  double cosH = (sin(-0.833 * RAD) - sin(lat * RAD) * sin(p.declinazione * RAD)) /
                (cos(lat * RAD) * cos(p.declinazione * RAD));
  if (cosH > 1) {
    r.sempreNotte = true;
    return r;
  }
  if (cosH < -1) {
    r.sempreGiorno = true;
    return r;
  }
  double h0 = acos(cosH) / RAD;
  double mezzogiorno = 12.0 - lon / 15.0 - p.eot / 60.0;
  r.alba = modulo24(mezzogiorno - h0 / 15.0);
  r.tramonto = modulo24(mezzogiorno + h0 / 15.0);
  return r;
}

}  // namespace sole
