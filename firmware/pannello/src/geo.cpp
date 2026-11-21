#include "geo.h"

#include <math.h>

namespace geo {
namespace {
constexpr double RAD = M_PI / 180.0;
constexpr double RAGGIO_KM = 6371.0;
}  // namespace

double distanzaKm(double lat1, double lon1, double lat2, double lon2) {
  double dlat = (lat2 - lat1) * RAD, dlon = (lon2 - lon1) * RAD;
  double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1 * RAD) * cos(lat2 * RAD) * sin(dlon / 2) * sin(dlon / 2);
  return 2 * RAGGIO_KM * atan2(sqrt(a), sqrt(1 - a));
}

double azimut(double lat1, double lon1, double lat2, double lon2) {
  double f1 = lat1 * RAD, f2 = lat2 * RAD, dl = (lon2 - lon1) * RAD;
  double y = sin(dl) * cos(f2);
  double x = cos(f1) * sin(f2) - sin(f1) * cos(f2) * cos(dl);
  double b = atan2(y, x) / RAD;
  return b < 0 ? b + 360 : b;
}

void intermedio(double lat1, double lon1, double lat2, double lon2, double f, double& lat, double& lon) {
  double f1 = lat1 * RAD, l1 = lon1 * RAD, f2 = lat2 * RAD, l2 = lon2 * RAD;
  double d = distanzaKm(lat1, lon1, lat2, lon2) / RAGGIO_KM;
  if (d < 1e-9) {
    lat = lat1;
    lon = lon1;
    return;
  }
  double a = sin((1 - f) * d) / sin(d), b = sin(f * d) / sin(d);
  double x = a * cos(f1) * cos(l1) + b * cos(f2) * cos(l2);
  double y = a * cos(f1) * sin(l1) + b * cos(f2) * sin(l2);
  double z = a * sin(f1) + b * sin(f2);
  lat = atan2(z, sqrt(x * x + y * y)) / RAD;
  lon = atan2(y, x) / RAD;
}

}  // namespace geo
