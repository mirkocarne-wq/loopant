// Preferences finto: memoria in RAM.
#pragma once
#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>

class Preferences {
 public:
  bool begin(const char* nome, bool) { spazio_ = nome; return true; }
  size_t getBytes(const char* k, void* buf, size_t n) {
    auto it = dati()[spazio_ + "/" + k];
    if (it.size() != n) return 0;
    memcpy(buf, it.data(), n);
    return n;
  }
  size_t putBytes(const char* k, const void* buf, size_t n) {
    dati()[spazio_ + "/" + k].assign((const char*)buf, (const char*)buf + n);
    return n;
  }
  uint8_t getUChar(const char* k, uint8_t d = 0) { uint8_t v; return getBytes(k, &v, 1) ? v : d; }
  uint16_t getUShort(const char* k, uint16_t d = 0) { uint16_t v; return getBytes(k, &v, 2) ? v : d; }
  size_t putUChar(const char* k, uint8_t v) { return putBytes(k, &v, 1); }
  size_t putUShort(const char* k, uint16_t v) { return putBytes(k, &v, 2); }
 private:
  static std::map<std::string, std::vector<char>>& dati() { static std::map<std::string, std::vector<char>> d; return d; }
  std::string spazio_;
};
