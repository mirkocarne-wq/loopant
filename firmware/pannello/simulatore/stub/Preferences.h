// Preferences finto: memoria in RAM.
#pragma once
#include <map>
#include <string>
#include <vector>
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
 private:
  static std::map<std::string, std::vector<char>>& dati() { static std::map<std::string, std::vector<char>> d; return d; }
  std::string spazio_;
};
