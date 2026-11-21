// Calcoli sul cerchio massimo: distanza, direzione e punti intermedi del percorso.
#pragma once

namespace geo {

double distanzaKm(double lat1, double lon1, double lat2, double lon2);
// Direzione iniziale (percorso corto), gradi 0-360 dal nord in senso orario.
double azimut(double lat1, double lon1, double lat2, double lon2);
// Punto a frazione f (0-1) del cerchio massimo tra i due punti.
void intermedio(double lat1, double lon1, double lat2, double lon2, double f, double& lat, double& lon);

}  // namespace geo
