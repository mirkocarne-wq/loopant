#include "qrz.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace qrz {
namespace {

// strstr limitato a [da, a)
const char* cerca(const char* da, const char* a, const char* s) {
  size_t n = strlen(s);
  for (const char* p = da; p + n <= a; p++)
    if (!memcmp(p, s, n)) return p;
  return nullptr;
}

void copia(char* out, size_t n, const char* s) {
  if (!n) return;
  strncpy(out, s ? s : "", n - 1);
  out[n - 1] = 0;
}

void aggiungi(char* out, size_t n, const char* s) {
  size_t l = strlen(out);
  if (l + 1 < n) copia(out + l, n - l, s);
}

// Scrive in out il carattere Unicode cp in UTF-8.
size_t utf8(uint32_t cp, char* out) {
  if (cp < 0x80) { out[0] = (char)cp; return 1; }
  if (cp < 0x800) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
  out[0] = (char)(0xE0 | (cp >> 12));
  out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[2] = (char)(0x80 | (cp & 0x3F));
  return 3;
}

// Decodifica le entità XML/HTML di [da, a) in out (anche in posto: out può coincidere con da).
size_t decodificaEntita(const char* da, const char* a, char* out, size_t n) {
  // Il testo decodificato non è mai più lungo dell'originale: si può decodificare in posto.
  size_t k = 0;
  for (const char* p = da; p < a && k + 1 < n;) {
    if (*p == '&') {
      const char* fine = (const char*)memchr(p, ';', (size_t)(a - p));
      if (fine && fine - p <= 10) {
        size_t l = (size_t)(fine - p - 1);
        const char* nome = p + 1;
        char c = 0;
        if (l == 2 && !memcmp(nome, "lt", 2)) c = '<';
        else if (l == 2 && !memcmp(nome, "gt", 2)) c = '>';
        else if (l == 3 && !memcmp(nome, "amp", 3)) c = '&';
        else if (l == 4 && !memcmp(nome, "quot", 4)) c = '"';
        else if (l == 4 && !memcmp(nome, "apos", 4)) c = '\'';
        if (c) {
          out[k++] = c;
          p = fine + 1;
          continue;
        }
        if (l >= 2 && nome[0] == '#') {
          uint32_t cp = nome[1] == 'x' || nome[1] == 'X' ? (uint32_t)strtoul(nome + 2, nullptr, 16)
                                                       : (uint32_t)strtoul(nome + 1, nullptr, 10);
          if (cp && k + 4 < n) {
            k += utf8(cp, out + k);
            p = fine + 1;
            continue;
          }
        }
      }
    }
    out[k++] = *p++;
  }
  out[k] = 0;
  return k;
}

int esadecimale(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// Decodifica URL (%XX e '+') di [da, a) in out.
size_t decodificaUrl(const char* da, const char* a, char* out, size_t n) {
  size_t k = 0;
  for (const char* p = da; p < a && k + 1 < n; p++) {
    if (*p == '%' && p + 2 < a && esadecimale(p[1]) >= 0 && esadecimale(p[2]) >= 0) {
      out[k++] = (char)(esadecimale(p[1]) * 16 + esadecimale(p[2]));
      p += 2;
    } else if (*p == '+') {
      out[k++] = ' ';
    } else {
      out[k++] = *p;
    }
  }
  out[k] = 0;
  return k;
}

void spazi(char* s) {  // toglie gli spazi ai bordi
  char* p = s;
  while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
  if (p != s) memmove(s, p, strlen(p) + 1);
  size_t l = strlen(s);
  while (l && (s[l - 1] == ' ' || s[l - 1] == '\n' || s[l - 1] == '\r' || s[l - 1] == '\t')) s[--l] = 0;
}

// Contenuto del tag <nome>…</nome> dentro [da, a), con le entità decodificate.
bool tag(const char* da, const char* a, const char* nome, char* out, size_t n) {
  char apri[24], chiudi[24];
  snprintf(apri, sizeof(apri), "<%s>", nome);
  snprintf(chiudi, sizeof(chiudi), "</%s>", nome);
  const char* p = cerca(da, a, apri);
  if (!p) return false;
  p += strlen(apri);
  const char* q = cerca(p, a, chiudi);
  if (!q) return false;
  decodificaEntita(p, q, out, n);
  spazi(out);
  return true;
}

bool si(const char* v) { return v[0] == 'Y' || v[0] == 'y' || v[0] == '1'; }

// --- Campi "CHIAVE=valore" della Logbook API -----------------------------------------------------

// Un separatore è '&' seguito da una chiave maiuscola e da '=' (le entità come &lt; sono minuscole).
bool separatore(const char* p) {
  if (*p != '&') return false;
  const char* q = p + 1;
  if (!(*q >= 'A' && *q <= 'Z')) return false;
  while ((*q >= 'A' && *q <= 'Z') || *q == '_' || (*q >= '0' && *q <= '9')) q++;
  return *q == '=';
}

// Trova il valore della chiave: [inizio, fine). Restituisce false se assente.
bool campo(const char* r, const char* chiave, const char*& inizio, const char*& fine) {
  size_t l = strlen(chiave);
  const char* p = r;
  while (*p) {
    // p è all'inizio di una coppia CHIAVE=valore
    const char* uguale = strchr(p, '=');
    if (!uguale) return false;
    const char* f = uguale + 1;
    while (*f && !separatore(f)) f++;
    if ((size_t)(uguale - p) == l && !memcmp(p, chiave, l)) {
      inizio = uguale + 1;
      fine = f;
      return true;
    }
    if (!*f) return false;
    p = f + 1;
  }
  return false;
}

bool campoTesto(const char* r, const char* chiave, char* out, size_t n) {
  const char *a, *b;
  if (!campo(r, chiave, a, b)) return false;
  decodificaUrl(a, b, out, n);
  return true;
}

// --- ADIF ----------------------------------------------------------------------------------------

bool uguali(const char* a, size_t la, const char* b) {
  if (strlen(b) != la) return false;
  for (size_t i = 0; i < la; i++)
    if (tolower((unsigned char)a[i]) != b[i]) return false;
  return true;
}

void campoAdif(Qso& q, const char* nome, size_t ln, const char* v, size_t lv, char* conferme) {
  char val[48];
  size_t l = lv < sizeof(val) - 1 ? lv : sizeof(val) - 1;
  memcpy(val, v, l);
  val[l] = 0;
  if (uguali(nome, ln, "call")) copia(q.call, sizeof(q.call), val);
  else if (uguali(nome, ln, "qso_date") && l >= 8) {
    snprintf(q.data, sizeof(q.data), "%.2s/%.2s/%.4s", val + 6, val + 4, val);
    memcpy(q.ordine, val, 8);
  } else if (uguali(nome, ln, "time_on") && l >= 4) {
    snprintf(q.ora, sizeof(q.ora), "%.2s:%.2s", val, val + 2);
    memcpy(q.ordine + 8, val, l >= 6 ? 6 : 4);
  } else if (uguali(nome, ln, "band")) {
    copia(q.banda, sizeof(q.banda), val);
    for (char* c = q.banda; *c; c++) *c = (char)tolower((unsigned char)*c);
  } else if (uguali(nome, ln, "mode")) {
    if (!q.modo[0]) copia(q.modo, sizeof(q.modo), val);
  } else if (uguali(nome, ln, "submode")) copia(q.modo, sizeof(q.modo), val);  // es. FT4 dentro MFSK
  else if (uguali(nome, ln, "freq")) q.freq_hz = (uint32_t)(atof(val) * 1e6 + 0.5);
  else if (uguali(nome, ln, "rst_sent")) copia(q.rstTx, sizeof(q.rstTx), val);
  else if (uguali(nome, ln, "rst_rcvd")) copia(q.rstRx, sizeof(q.rstRx), val);
  else if (uguali(nome, ln, "app_qrzlog_logid")) q.logid = (uint32_t)strtoul(val, nullptr, 10);
  else if (uguali(nome, ln, "app_qrzlog_status") && (val[0] == 'C' || val[0] == 'c')) *conferme = 1;
  else if ((uguali(nome, ln, "qsl_rcvd") || uguali(nome, ln, "lotw_qsl_rcvd")) && si(val)) *conferme = 1;
}

uint16_t leggiAdif(const char* s, Qso* qso, uint16_t max) {
  uint16_t n = 0;
  Qso q;
  memset(&q, 0, sizeof(q));
  char conferme = 0;
  bool vuoto = true;
  const char* p = s;
  while ((p = strchr(p, '<')) && n < max) {
    const char* fine = strchr(p, '>');
    if (!fine) break;
    const char* nome = p + 1;
    const char* due = (const char*)memchr(nome, ':', (size_t)(fine - nome));
    size_t ln = (size_t)((due ? due : fine) - nome);
    if (uguali(nome, ln, "eor")) {
      if (!vuoto) {
        q.confermato = conferme;
        if (!q.ordine[0]) memset(q.ordine, '0', sizeof(q.ordine) - 1);
        qso[n++] = q;
      }
      memset(&q, 0, sizeof(q));
      conferme = 0;
      vuoto = true;
      p = fine + 1;
      continue;
    }
    if (uguali(nome, ln, "eoh")) {  // fine dell'intestazione: si ricomincia
      memset(&q, 0, sizeof(q));
      conferme = 0;
      vuoto = true;
      p = fine + 1;
      continue;
    }
    size_t lv = due ? (size_t)strtoul(due + 1, nullptr, 10) : 0;
    const char* v = fine + 1;
    size_t disponibili = strlen(v);
    if (lv > disponibili) lv = disponibili;
    if (due) {
      campoAdif(q, nome, ln, v, lv, &conferme);
      vuoto = false;
    }
    p = v + lv;
  }
  return n;
}

}  // namespace

// --- XML Data ------------------------------------------------------------------------------------

bool analizzaXml(const char* xml, Sessione& s, Nominativo* n) {
  memset(&s, 0, sizeof(s));
  const char* fine = xml + strlen(xml);
  const char* ss = cerca(xml, fine, "<Session>");
  const char* se = ss ? cerca(ss, fine, "</Session>") : nullptr;
  if (ss && se) {
    s.valida = tag(ss, se, "Key", s.chiave, sizeof(s.chiave)) && s.chiave[0];
    tag(ss, se, "SubExp", s.scadenza, sizeof(s.scadenza));
    s.abbonato = s.scadenza[0] && strcmp(s.scadenza, "non-subscriber") != 0;
    tag(ss, se, "Error", s.errore, sizeof(s.errore));
    tag(ss, se, "Message", s.messaggio, sizeof(s.messaggio));
  } else {
    copia(s.errore, sizeof(s.errore), "Risposta di QRZ non riconosciuta");
  }

  const char* cs = cerca(xml, fine, "<Callsign>");
  const char* ce = cs ? cerca(cs, fine, "</Callsign>") : nullptr;
  if (!cs || !ce) return false;
  if (!n) return true;
  memset(n, 0, sizeof(*n));
  char b[96], c[96];
  tag(cs, ce, "call", n->call, sizeof(n->call));
  if (!tag(cs, ce, "name_fmt", n->nome, sizeof(n->nome)) || !n->nome[0]) {
    b[0] = c[0] = 0;
    tag(cs, ce, "fname", b, sizeof(b));
    tag(cs, ce, "name", c, sizeof(c));
    snprintf(n->nome, sizeof(n->nome), "%s%s%s", b, b[0] && c[0] ? " " : "", c);
  }
  b[0] = c[0] = 0;
  tag(cs, ce, "addr2", b, sizeof(b));
  tag(cs, ce, "state", c, sizeof(c));
  snprintf(n->qth, sizeof(n->qth), "%s%s%s", b, b[0] && c[0] ? ", " : "", c);
  if (!tag(cs, ce, "country", n->paese, sizeof(n->paese)) || !n->paese[0])
    tag(cs, ce, "land", n->paese, sizeof(n->paese));
  tag(cs, ce, "grid", n->locator, sizeof(n->locator));
  char lat[24] = "", lon[24] = "";
  if (tag(cs, ce, "lat", lat, sizeof(lat)) && tag(cs, ce, "lon", lon, sizeof(lon)) && lat[0] && lon[0]) {
    n->lat = atof(lat);
    n->lon = atof(lon);
    n->coordinate = true;
  }
  if (tag(cs, ce, "dxcc", b, sizeof(b))) n->dxcc = atoi(b);
  if (tag(cs, ce, "cqzone", b, sizeof(b))) n->cq = atoi(b);
  if (tag(cs, ce, "ituzone", b, sizeof(b))) n->itu = atoi(b);
  // QSL
  const char* sep = "";
  if (tag(cs, ce, "lotw", b, sizeof(b)) && si(b)) { aggiungi(n->qsl, sizeof(n->qsl), "LoTW"); sep = " · "; }
  if (tag(cs, ce, "eqsl", b, sizeof(b)) && si(b)) {
    aggiungi(n->qsl, sizeof(n->qsl), sep);
    aggiungi(n->qsl, sizeof(n->qsl), "eQSL");
    sep = " · ";
  }
  if (tag(cs, ce, "mqsl", b, sizeof(b)) && si(b)) {
    aggiungi(n->qsl, sizeof(n->qsl), sep);
    aggiungi(n->qsl, sizeof(n->qsl), "carta");
    sep = " · ";
  }
  if (tag(cs, ce, "qslmgr", b, sizeof(b)) && b[0] && strcmp(b, "NONE") && strcmp(b, "none")) {
    aggiungi(n->qsl, sizeof(n->qsl), sep);
    aggiungi(n->qsl, sizeof(n->qsl), "via ");
    aggiungi(n->qsl, sizeof(n->qsl), b);
  }
  tag(cs, ce, "class", n->classe, sizeof(n->classe));
  tag(cs, ce, "image", n->immagine, sizeof(n->immagine));
  return true;
}

// --- Logbook API ---------------------------------------------------------------------------------

uint16_t analizzaLogbook(const char* r, Logbook& lb, Qso* qso, uint16_t max) {
  memset(&lb, 0, sizeof(lb));
  campoTesto(r, "RESULT", lb.result, sizeof(lb.result));
  spazi(lb.result);
  campoTesto(r, "REASON", lb.reason, sizeof(lb.reason));
  // QRZ a volte manda "API USER: … STATUS: AUTH REASON: invalid api key EXTENDED: …": si tiene solo il motivo
  char* m = strstr(lb.reason, "REASON:");
  if (m) {
    memmove(lb.reason, m + 7, strlen(m + 7) + 1);
    char* ext = strstr(lb.reason, "EXTENDED:");
    if (ext) *ext = 0;
  }
  spazi(lb.reason);
  char c[24];
  if (campoTesto(r, "COUNT", c, sizeof(c))) {
    lb.count = (uint32_t)strtoul(c, nullptr, 10);
    lb.haCount = true;
  }
  const char *a, *b;
  if (!qso || !max || !campo(r, "ADIF", a, b)) return 0;
  size_t l = (size_t)(b - a);
  char* adif = (char*)malloc(l + 1);
  if (!adif) return 0;
  // ADIF codificato come URL (%3C...) oppure con entità HTML (&lt;...): si decodificano entrambi
  bool url = cerca(a, b, "%3C") || cerca(a, b, "%3c");
  size_t k = url ? decodificaUrl(a, b, adif, l + 1) : (memcpy(adif, a, l), adif[l] = 0, l);
  decodificaEntita(adif, adif + k, adif, k + 1);
  uint16_t n = leggiAdif(adif, qso, max);
  free(adif);
  return n;
}

uint16_t ultimiLogid(const char* r, uint32_t* out, uint16_t n) {
  const char *a, *b;
  if (!n || !campo(r, "LOGIDS", a, b)) return 0;
  uint16_t k = 0;
  const char* p = a;
  while (p < b) {
    while (p < b && !(*p >= '0' && *p <= '9')) {
      if (*p == '%' && p + 2 < b) p += 3;  // %2C
      else p++;
    }
    if (p >= b) break;
    uint32_t v = 0;
    while (p < b && *p >= '0' && *p <= '9') v = v * 10 + (uint32_t)(*p++ - '0');
    // inserimento in ordine decrescente tra gli n più alti
    if (k < n || v > out[k - 1]) {
      uint16_t i = k < n ? k++ : (uint16_t)(n - 1);
      while (i > 0 && out[i - 1] < v) {
        out[i] = out[i - 1];
        i--;
      }
      out[i] = v;
    }
  }
  return k;
}

bool codificaUrl(const char* in, char* out, size_t n) {
  static const char* HEX = "0123456789ABCDEF";
  size_t k = 0;
  for (const unsigned char* p = (const unsigned char*)in; *p; p++) {
    bool libero = isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~';
    if (k + (libero ? 1 : 3) >= n) {
      out[k] = 0;
      return false;
    }
    if (libero) out[k++] = (char)*p;
    else {
      out[k++] = '%';
      out[k++] = HEX[*p >> 4];
      out[k++] = HEX[*p & 15];
    }
  }
  out[k] = 0;
  return true;
}

void ordinaRecenti(Qso* q, uint16_t n) {
  for (uint16_t i = 1; i < n; i++) {
    Qso v = q[i];
    int j = i - 1;
    while (j >= 0 && strcmp(q[j].ordine, v.ordine) < 0) {
      q[j + 1] = q[j];
      j--;
    }
    q[j + 1] = v;
  }
}

bool serveAbbonamento(const Logbook& lb) {
  if (!strcmp(lb.result, "AUTH")) return true;
  char r[sizeof(lb.reason)];
  copia(r, sizeof(r), lb.reason);
  for (char* c = r; *c; c++) *c = (char)tolower((unsigned char)*c);
  return strstr(r, "subscri") != nullptr;
}

}  // namespace qrz
