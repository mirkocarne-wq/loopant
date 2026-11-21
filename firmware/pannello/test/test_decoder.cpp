// Test sul PC dei decoder CW e RTTY con segnali sintetici e rumore gaussiano.
// g++ -std=c++17 -O2 -I../src test_decoder.cpp ../src/decoder/cw.cpp ../src/decoder/rtty.cpp -o test_decoder
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <random>
#include <string>
#include <vector>

#include "decoder/cw.h"
#include "decoder/rtty.h"

static const float FS = 8000;
static std::mt19937 rng(12345);
static int errori = 0;

static void verifica(bool ok, const std::string& cosa, const std::string& dettaglio = "") {
  printf("%s %s%s%s\n", ok ? "OK  " : "ERRORE", cosa.c_str(), dettaglio.empty() ? "" : " -> ", dettaglio.c_str());
  if (!ok) errori++;
}

static std::vector<int16_t> aPcm(const std::vector<float>& s, float rumore) {
  std::normal_distribution<float> g(0, rumore);
  std::vector<int16_t> out(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    float v = s[i] + g(rng);
    if (v > 0.99f) v = 0.99f;
    if (v < -0.99f) v = -0.99f;
    out[i] = (int16_t)(v * 32767);
  }
  return out;
}

static std::string pulisci(const std::string& s) {  // spazi multipli e ai bordi
  std::string o;
  for (char c : s) {
    if (c == ' ' && (o.empty() || o.back() == ' ')) continue;
    o += c;
  }
  while (!o.empty() && o.back() == ' ') o.pop_back();
  return o;
}

// --- CW ------------------------------------------------------------------------------------------

static const char* morse(char c) {
  static const char* L[] = {".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",
                            ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",
                            "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--.."};
  static const char* D[] = {"-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."};
  if (c >= 'A' && c <= 'Z') return L[c - 'A'];
  if (c >= '0' && c <= '9') return D[c - '0'];
  if (c == '?') return "..--..";
  if (c == '/') return "-..-.";
  return nullptr;
}

static std::vector<float> generaCw(const std::string& testo, float wpm, float f, float ampiezza) {
  std::vector<float> s;
  float dit = 1.2f / wpm;
  double fase = 0;
  auto tono = [&](float durata, bool acceso) {
    int n = (int)(durata * FS);
    int rampa = (int)(0.005f * FS);
    for (int i = 0; i < n; i++) {
      float a = 0;
      if (acceso) {
        a = ampiezza;
        if (i < rampa) a *= 0.5f - 0.5f * cosf((float)M_PI * i / rampa);
        if (i > n - rampa) a *= 0.5f - 0.5f * cosf((float)M_PI * (n - i) / rampa);
      }
      s.push_back(a * (float)sin(fase));
      fase += 2 * M_PI * f / FS;
    }
  };
  tono(0.5f, false);
  for (char c : testo) {
    if (c == ' ') {
      tono(4 * dit, false);  // 3 già messi dopo il carattere + 4 = 7
      continue;
    }
    const char* m = morse(c);
    for (const char* p = m; *p; p++) {
      tono(*p == '.' ? dit : 3 * dit, true);
      tono(dit, false);
    }
    tono(2 * dit, false);  // 1 + 2 = 3 tra i caratteri
  }
  tono(1.0f, false);
  return s;
}

static void raccogli(char c, void* ctx) { ((std::string*)ctx)->push_back(c); }

static std::string decodificaCw(const std::vector<int16_t>& pcm, float tono, float filtro, uint8_t wpmFisso,
                                float* wpmStimato = nullptr) {
  std::string out;
  decoder::DecoderCw d;
  d.configura(FS, tono, filtro, wpmFisso, raccogli, &out);
  for (size_t i = 0; i < pcm.size(); i += 256) d.campioni(&pcm[i], std::min<size_t>(256, pcm.size() - i));
  if (wpmStimato) *wpmStimato = d.wpm();
  return pulisci(out);
}

// --- RTTY ----------------------------------------------------------------------------------------

static bool codiceBaudot(char c, uint8_t& codice, bool& cifra) {
  static const char LET[32] = {0,   'E', '\n', 'A', ' ', 'S', 'I', 'U', '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
                               'T', 'Z', 'L',  'W', 'H', 'Y', 'P', 'Q', 'O',  'B', 'G', 0,   'M', 'X', 'V', 0};
  static const char CIF[32] = {0,   '3', '\n', '-', ' ', '\'', '8', '7', '\r', '$', '4', '\'', ',', '!', ':', '(',
                               '5', '"', ')',  '2', '#', '6',  '0', '1', '9',  '?', '&', 0,    '.', '/', ';', 0};
  for (int i = 0; i < 32; i++)
    if (LET[i] == c) { codice = i; cifra = false; return true; }
  for (int i = 0; i < 32; i++)
    if (CIF[i] == c) { codice = i; cifra = true; return true; }
  return false;
}

static std::vector<float> generaRtty(const std::string& testo, float baud, float shift, float mark, float stop,
                                     float offset, bool invertito, float ampiezza) {
  std::vector<float> s;
  double fase = 0, t = 0;
  float T = FS / baud;
  auto bitm = [&](bool m, float bits) {
    bool tonoMark = invertito ? !m : m;
    float f = (tonoMark ? mark : mark + shift) + offset;
    t += bits * T;
    while ((double)s.size() < t) {
      s.push_back(ampiezza * (float)sin(fase));
      fase += 2 * M_PI * f / FS;
    }
  };
  auto car = [&](uint8_t c) {
    bitm(false, 1);
    for (int i = 0; i < 5; i++) bitm((c >> i) & 1, 1);
    bitm(true, stop);
  };
  bitm(true, 30);  // riposo in mark
  bool cifre = false;
  car(0x1F);
  for (char c : testo) {
    uint8_t k;
    bool fig;
    if (!codiceBaudot(c, k, fig)) continue;
    if (c != ' ' && fig != cifre) {
      car(fig ? 0x1B : 0x1F);
      cifre = fig;
    }
    car(k);
    if (c == ' ') cifre = false;  // il trasmettitore usa UOS come il ricevitore
  }
  bitm(true, 30);
  return s;
}

static std::string decodificaRtty(const std::vector<int16_t>& pcm, const decoder::DecoderRtty::Parametri& p,
                                  float* offset = nullptr) {
  std::string out;
  decoder::DecoderRtty d;
  d.configura(FS, p, raccogli, &out);
  for (size_t i = 0; i < pcm.size(); i += 256) d.campioni(&pcm[i], std::min<size_t>(256, pcm.size() - i));
  if (offset) *offset = d.offsetAfc();
  return pulisci(out);
}

int main() {
  const std::string cq = "CQ CQ DE TEST TEST K";

  // CW
  {
    std::string r = decodificaCw(aPcm(generaCw(cq, 18, 700, 0.3f), 0.05f), 700, 250, 0);
    verifica(r == cq, "CW 18 WPM, 700 Hz, rumore moderato", "\"" + r + "\"");
  }
  {
    float wpm = 0;
    std::string testo = "PARIS PARIS PARIS PARIS PARIS";
    std::string r = decodificaCw(aPcm(generaCw(testo, 28, 650, 0.3f), 0.05f), 650, 250, 0, &wpm);
    char buf[64];
    snprintf(buf, sizeof(buf), "\"%s\", stima %.1f WPM", r.c_str(), wpm);
    verifica(r.size() >= 11 && r.substr(r.size() - 11) == "PARIS PARIS" && fabsf(wpm - 28) < 3,
             "CW 28 WPM con stima automatica della velocità", buf);
  }
  {
    std::string r = decodificaCw(aPcm(generaCw(cq, 12, 800, 0.3f), 0.12f), 800, 100, 12);
    verifica(r == cq, "CW 12 WPM fissi, filtro 100 Hz, molto rumore", "\"" + r + "\"");
  }
  {
    std::string r = decodificaCw(aPcm(generaCw("5NN 599 TU", 22, 700, 0.3f), 0.05f), 700, 500, 0);
    verifica(r == "5NN 599 TU", "CW 22 WPM con cifre, filtro 500 Hz", "\"" + r + "\"");
  }

  // RTTY
  const std::string msg = "RYRYRY CQ CQ DE TEST TEST 599 73 K";
  decoder::DecoderRtty::Parametri p;
  {
    std::string r = decodificaRtty(aPcm(generaRtty(msg, 45.45f, 170, 2125, 1.5f, 0, false, 0.3f), 0.05f), p);
    verifica(r == msg, "RTTY 45,45 baud, shift 170 Hz, stop 1,5", "\"" + r + "\"");
  }
  {
    decoder::DecoderRtty::Parametri q = p;
    q.reverse = true;
    std::string r = decodificaRtty(aPcm(generaRtty(msg, 45.45f, 170, 2125, 1.5f, 0, true, 0.3f), 0.05f), q);
    verifica(r == msg, "RTTY con segnale invertito e Reverse attivo", "\"" + r + "\"");
  }
  {
    decoder::DecoderRtty::Parametri q = p;
    q.baud = 50;
    q.shift = 850;
    q.mark = 1275;
    q.bitStop = 1;
    std::string r = decodificaRtty(aPcm(generaRtty(msg, 50, 850, 1275, 1, 0, false, 0.3f), 0.05f), q);
    verifica(r == msg, "RTTY 50 baud, shift 850 Hz, stop 1", "\"" + r + "\"");
  }
  {
    float off = 0;
    std::string r = decodificaRtty(aPcm(generaRtty(msg + " " + msg, 45.45f, 170, 2125, 1.5f, 25, false, 0.3f), 0.05f),
                                   p, &off);
    char buf[96];
    snprintf(buf, sizeof(buf), "AFC %.1f Hz, \"%s\"", off, r.c_str());
    bool finale = r.size() >= msg.size() && r.substr(r.size() - msg.size()) == msg;
    verifica(finale && fabsf(off - 25) < 6, "RTTY con 25 Hz di errore e AFC", buf);
  }
  {
    std::string r = decodificaRtty(aPcm(generaRtty(msg, 45.45f, 170, 2125, 1.5f, 0, false, 0.3f), 0.15f), p);
    int diversi = 0;
    for (size_t i = 0; i < std::min(r.size(), msg.size()); i++) diversi += r[i] != msg[i];
    diversi += (int)(r.size() > msg.size() ? r.size() - msg.size() : msg.size() - r.size());
    verifica(diversi <= 3, "RTTY con molto rumore (al massimo 3 caratteri sbagliati)", "\"" + r + "\"");
  }

  printf(errori ? "\n%d ERRORI\n" : "\nTutti i test superati\n", errori);
  return errori ? 1 : 0;
}
