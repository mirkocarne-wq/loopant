// Locator Maidenhead (4 o 6 caratteri, es. JN61 o JN61fv) -> latitudine/longitudine del centro.
#pragma once

namespace locator {

bool aCoordinate(const char* loc, double& lat, double& lon);

}  // namespace locator
