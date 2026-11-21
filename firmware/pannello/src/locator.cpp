#include "locator.h"

#include <ctype.h>
#include <string.h>

namespace locator {

bool aCoordinate(const char* loc, double& lat, double& lon) {
  if (!loc) return false;
  size_t n = strlen(loc);
  if (n != 4 && n != 6) return false;
  char c0 = (char)toupper((unsigned char)loc[0]), c1 = (char)toupper((unsigned char)loc[1]);
  if (c0 < 'A' || c0 > 'R' || c1 < 'A' || c1 > 'R') return false;
  if (!isdigit((unsigned char)loc[2]) || !isdigit((unsigned char)loc[3])) return false;
  lon = (c0 - 'A') * 20.0 - 180.0 + (loc[2] - '0') * 2.0;
  lat = (c1 - 'A') * 10.0 - 90.0 + (loc[3] - '0') * 1.0;
  if (n == 4) {
    lon += 1.0;  // centro del quadrato
    lat += 0.5;
    return true;
  }
  char s0 = (char)tolower((unsigned char)loc[4]), s1 = (char)tolower((unsigned char)loc[5]);
  if (s0 < 'a' || s0 > 'x' || s1 < 'a' || s1 > 'x') return false;
  lon += (s0 - 'a') * (5.0 / 60.0) + 2.5 / 60.0;
  lat += (s1 - 'a') * (2.5 / 60.0) + 1.25 / 60.0;
  return true;
}

}  // namespace locator
