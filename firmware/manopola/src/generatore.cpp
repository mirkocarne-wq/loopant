#include "generatore.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace generatore {
namespace {

const char* TESTO = "CQ CQ CQ DE LOOPANT LOOPANT PSE K   RYRYRY THE QUICK BROWN FOX 0123456789   ";
const float AMPIEZZA = 8000;  // circa -12 dB dal fondo scala
const float RUMORE = 600;

Parametri p;
double fase = 0;
size_t posTesto = 0;

// --- CW: elementi da suonare (durata in dit, acceso/spento)
struct Elemento {
  float dit;
  bool acceso;
};
Elemento coda[16];
int nCoda = 0, iCoda = 0;
float rimasti = 0;  // campioni dell'elemento in corso
float durata = 0;   // campioni totali dell'elemento in corso
bool acceso = false;

const char* morse(char c) {
  static const char* L[] = {".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",
                            ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",
                            "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--.."};
  static const char* D[] = {"-----", ".----", "..---", "...--", "....-",
                            ".....", "-....", "--...", "---..", "----."};
  if (c >= 'A' && c <= 'Z') return L[c - 'A'];
  if (c >= '0' && c <= '9') return D[c - '0'];
  return nullptr;
}

void prossimoCarattereCw() {
  nCoda = iCoda = 0;
  char c = TESTO[posTesto];
  posTesto = TESTO[posTesto + 1] ? posTesto + 1 : 0;
  const char* m = morse(c);
  if (!m) {  // spazio: 7 dit in tutto (3 già dopo il carattere precedente)
    coda[nCoda++] = {4, false};
    return;
  }
  for (const char* q = m; *q; q++) {
    coda[nCoda++] = {*q == '.' ? 1.0f : 3.0f, true};
    coda[nCoda++] = {1, false};
  }
  coda[nCoda - 1].dit = 3;  // 3 dit tra i caratteri
}

float campioneCw() {
  if (rimasti <= 0) {
    if (iCoda >= nCoda) prossimoCarattereCw();
    Elemento e = coda[iCoda++];
    durata = rimasti = e.dit * 1.2f / p.wpm * p.fs;
    acceso = e.acceso;
  }
  float a = 0;
  if (acceso) {  // fronti a coseno rialzato di 5 ms
    float rampa = 0.005f * p.fs, t = durata - rimasti;
    a = 1;
    if (t < rampa) a = 0.5f - 0.5f * cosf((float)M_PI * t / rampa);
    if (rimasti < rampa) a = 0.5f - 0.5f * cosf((float)M_PI * rimasti / rampa);
  }
  rimasti--;
  float s = a * (float)sin(fase);
  fase += 2 * M_PI * p.tono / p.fs;
  if (fase > 2 * M_PI) fase -= 2 * M_PI;
  return s;
}

// --- RTTY: bit da trasmettere (mark/space, durata in bit)
struct Bit {
  bool mark;
  float bit;
};
Bit bits[24];
int nBit = 0, iBit = 0;
float rimastiBit = 0;
bool markAttuale = true;
bool cifre = false;

bool baudot(char c, uint8_t& codice, bool& cifra) {
  static const char LET[32] = {0,   'E', '\n', 'A', ' ', 'S', 'I', 'U', '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
                               'T', 'Z', 'L',  'W', 'H', 'Y', 'P', 'Q', 'O',  'B', 'G', 0,   'M', 'X', 'V', 0};
  static const char CIF[32] = {0,   '3', '\n', '-', ' ', '\'', '8', '7', '\r', '$', '4', '\'', ',', '!', ':', '(',
                               '5', '"', ')',  '2', '#', '6',  '0', '1', '9',  '?', '&', 0,    '.', '/', ';', 0};
  for (int i = 0; i < 32; i++)
    if (LET[i] == c) {
      codice = i;
      cifra = false;
      return true;
    }
  for (int i = 0; i < 32; i++)
    if (CIF[i] == c) {
      codice = i;
      cifra = true;
      return true;
    }
  return false;
}

void accodaCarattere(uint8_t c) {
  bits[nBit++] = {false, 1};  // start
  for (int i = 0; i < 5; i++) bits[nBit++] = {(bool)((c >> i) & 1), 1};
  bits[nBit++] = {true, p.bitStop};
}

void prossimoCarattereRtty() {
  nBit = iBit = 0;
  char c = TESTO[posTesto];
  posTesto = TESTO[posTesto + 1] ? posTesto + 1 : 0;
  uint8_t k;
  bool fig;
  if (!baudot(c, k, fig)) {
    bits[nBit++] = {true, 1};
    return;
  }
  if (c != ' ' && fig != cifre) {
    accodaCarattere(fig ? 0x1B : 0x1F);
    cifre = fig;
  }
  accodaCarattere(k);
  if (c == ' ') cifre = false;  // come il ricevitore con UOS
}

float campioneRtty() {
  if (rimastiBit <= 0) {
    if (iBit >= nBit) prossimoCarattereRtty();
    Bit b = bits[iBit++];
    rimastiBit += b.bit * p.fs / p.baud;
    markAttuale = b.mark;
  }
  rimastiBit--;
  float f = markAttuale ? p.mark : p.mark + p.shift;
  float s = (float)sin(fase);
  fase += 2 * M_PI * f / p.fs;  // fase continua
  if (fase > 2 * M_PI) fase -= 2 * M_PI;
  return s;
}

}  // namespace

void configura(const Parametri& nuovi) {
  p = nuovi;
  if (p.wpm < 5) p.wpm = 18;
  nCoda = iCoda = nBit = iBit = 0;
  rimasti = rimastiBit = 0;
  posTesto = 0;
  cifre = false;
}

void campioni(int16_t* x, size_t n) {
  for (size_t i = 0; i < n; i++) {
    float s = p.rtty ? campioneRtty() : campioneCw();
    float r = ((rand() & 0xFFFF) / 32768.0f - 1.0f) * RUMORE;
    float v = s * AMPIEZZA + r;
    x[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
  }
}

}  // namespace generatore
