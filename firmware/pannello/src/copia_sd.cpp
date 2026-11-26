#include "copia_sd.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <bande.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "impostazioni.h"
#include "link_antenna.h"
#include "memorie.h"
#include "ora.h"
#include "waveshare/io_extension.h"
#include "waveshare/lvgl_port.h"

namespace copia_sd {
namespace {

// Slot microSD del 7B e del 5B: SPI su GPIO 11/12/13, CS sull'IO expander (EXIO4), tenuto basso: la SD è
// l'unico dispositivo sul bus (come nell'esempio Waveshare 03_SD_Test).
constexpr int PIN_MOSI = 11, PIN_SCK = 12, PIN_MISO = 13;
constexpr const char* CARTELLA = "/loopant";
constexpr const char* FILE_COPIA = "/loopant/copia.txt";
constexpr int FORMATO = 1;
constexpr uint32_t ATTESA_TABELLA_MS = 6000;  // lettura di una tabella dall'antenna
constexpr uint32_t ATTESA_COMANDO_MS = 4000;  // risposta a CAL_CANCELLA / CAL_SCRIVI (con le ripetizioni)
constexpr uint32_t RIAVVIO_DOPO_MS = 3000;

enum class Richiesta : uint8_t { NESSUNA, SALVA, RIPRISTINA };
enum class Passo : uint8_t { FERMO, LEGGI, SCRIVI, VERIFICA };

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Richiesta richiesta = Richiesta::NESSUNA;
Stato st = {Fase::PRONTA, "Nessuna operazione"};

struct TabellaCopia {
  bande::Banda banda;
  uint8_t n;
  uint32_t f[link_antenna::MAX_PUNTI];
  int32_t p[link_antenna::MAX_PUNTI];
};
TabellaCopia tabelle[bande::NUMERO];
uint8_t nTabelle = 0;
bool tabelleTx = false;  // tipo dell'antenna delle tabelle

Passo passo = Passo::FERMO;
uint8_t iTabella = 0;
int iPunto = -1;  // -1: prima la cancellazione della tabella
bool attesa = false;
bool attesaScrivi = false;  // il comando in attesa è un CAL_SCRIVI (l'unico che può essere rifiutato)
uint32_t inizio = 0;
uint16_t punti = 0, errori = 0;
uint32_t riavvioAlle = 0;

void imposta(Fase f, const char* fmt, ...) {
  char buf[sizeof(st.testo)];
  va_list a;
  va_start(a, fmt);
  vsnprintf(buf, sizeof(buf), fmt, a);
  va_end(a);
  portENTER_CRITICAL(&mux);
  st.fase = f;
  memcpy(st.testo, buf, sizeof(st.testo));
  portEXIT_CRITICAL(&mux);
  Serial.printf("microSD: %s\n", buf);
}

bool richiedi(Richiesta r) {
  portENTER_CRITICAL(&mux);
  bool libera = st.fase != Fase::IN_CORSO && richiesta == Richiesta::NESSUNA;
  if (libera) richiesta = r;
  portEXIT_CRITICAL(&mux);
  return libera;
}

const char* nomeBanda(bande::Banda b) {
  const bande::Info* i = bande::info(b);
  return i ? i->nome : "?";
}

// --- Scheda ----------------------------------------------------------------------------------
// Sempre con il lucchetto di LVGL: l'IO expander (CS) e l'I2C sono anche del task dell'interfaccia.

bool monta() {
  IO_EXTENSION_Output(IO_EXTENSION_IO_4, 0);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, -1);
  return SD.begin(0xFF, SPI, 4000000, "/sd", 2);
}

void smonta() { SD.end(); }

bool scriviSuFile(const char* nome, const String& testo) {
  File f = SD.open(nome, FILE_WRITE);
  if (!f) return false;
  size_t n = f.print(testo);
  f.close();
  return n == testo.length();
}

// --- Contenuto del file ----------------------------------------------------------------------

String componi(bool conTabelle) {
  const Impostazioni& imp = impostazioni::get();
  String s;
  s.reserve(4096);
  char buf[96];
  s += "# loopant - copia del pannello: impostazioni, memorie personali, calibrazione dell'antenna\n";
  if (ora::valida()) {
    time_t t = time(nullptr);
    struct tm u;
    gmtime_r(&t, &u);
    strftime(buf, sizeof(buf), "# %d/%m/%Y %H:%M UTC\n", &u);
    s += buf;
  }
  s += "# Password del WiFi e di QRZ non salvate: dopo il ripristino vanno reinserite.\n";
  s += "formato=" + String(FORMATO) + "\n";
  if (conTabelle) s += String("antenna=") + (tabelleTx ? "tx" : "rx") + "\n";

  s += "\n[impostazioni]\n";
  s += String("nominativo=") + imp.nominativo + "\n";
  s += String("locator=") + imp.locator + "\n";
  s += "tema=" + String(imp.tema) + "\n";
  s += "screensaver=" + String(imp.screensaver) + "\n";
  s += "attesa_min=" + String(imp.attesa_min) + "\n";
  s += "luminosita=" + String(imp.luminosita) + "\n";
  s += "modo_ora=" + String(imp.modoOra) + "\n";
  s += "fuso=" + String(imp.fuso) + "\n";
  s += String("wifi_rete=") + imp.wifiSsid + "\n";
  s += "dec_modo=" + String(imp.decModo) + "\n";
  s += "cw_tono=" + String(imp.cwTono) + "\n";
  s += "cw_filtro=" + String(imp.cwFiltro) + "\n";
  s += "cw_wpm=" + String(imp.cwWpm) + "\n";
  s += "rtty_baud=" + String(imp.rttyBaud) + "\n";
  s += "rtty_shift=" + String(imp.rttyShift) + "\n";
  s += "rtty_mark=" + String(imp.rttyMark) + "\n";
  s += "rtty_stop=" + String(imp.rttyStop) + "\n";
  s += "rtty_opzioni=" + String(imp.rttyOpzioni) + "\n";
  s += String("qrz_utente=") + impostazioni::qrz().utente + "\n";

  s += "\n[ultime]\n";
  s += "usata=" + String((unsigned long)impostazioni::ultimaUsata()) + "\n";
  for (const bande::Info& b : bande::ELENCO)
    s += String((int)b.banda) + "=" + String((unsigned long)impostazioni::ultima(b.banda)) + "\n";

  s += "\n[memorie]\n# nome=frequenza in Hz\n";
  for (uint8_t i = 0; i < memorie::numeroPersonali(); i++) {
    const memorie::Memoria& m = memorie::personale(i);
    s += String(m.nome) + "=" + String((unsigned long)m.freq_hz) + "\n";
  }

  if (conTabelle) {
    s += "\n[calibrazione]\n# banda=<metri>, poi un punto per riga: frequenza in Hz e posizione in passi\n";
    for (uint8_t i = 0; i < nTabelle; i++) {
      s += "banda=" + String((int)tabelle[i].banda) + "\n";
      for (uint8_t j = 0; j < tabelle[i].n; j++)
        s += String((unsigned long)tabelle[i].f[j]) + " " + String((long)tabelle[i].p[j]) + "\n";
    }
  }
  return s;
}

void scriviCopia(bool conTabelle) {
  String testo = componi(conTabelle);
  char datato[40] = "";
  if (ora::valida()) {
    time_t t = time(nullptr);
    struct tm u;
    gmtime_r(&t, &u);
    strftime(datato, sizeof(datato), "/loopant/copia-%Y%m%d-%H%M.txt", &u);
  }
  bool ok = false, montata = false;
  if (lvgl_port_lock(-1)) {
    montata = monta();
    if (montata) {
      SD.mkdir(CARTELLA);
      ok = scriviSuFile(FILE_COPIA, testo);
      if (ok && datato[0]) ok = scriviSuFile(datato, testo);
      smonta();
    }
    lvgl_port_unlock();
  }
  if (!montata) {
    imposta(Fase::ERRORE, "microSD non trovata: inseriscila (FAT32) e riprova");
    return;
  }
  if (!ok) {
    imposta(Fase::ERRORE, "Scrittura sulla microSD non riuscita");
    return;
  }
  uint16_t tot = 0;
  for (uint8_t i = 0; i < nTabelle; i++) tot += tabelle[i].n;
  if (conTabelle)
    imposta(Fase::FINITA, "Copia salvata in %s: impostazioni, %u memorie, calibrazione %s (%u punti)", FILE_COPIA,
            memorie::numeroPersonali(), tabelleTx ? "TX" : "RX", tot);
  else
    imposta(Fase::FINITA, "Copia salvata in %s: impostazioni e %u memorie. Calibrazione NON salvata: antenna non "
            "collegata", FILE_COPIA, memorie::numeroPersonali());
}

// --- Salvataggio -----------------------------------------------------------------------------

void avviaSalvataggio() {
  link_antenna::Stato s = link_antenna::stato();
  if (!s.collegato) {  // solo pannello
    nTabelle = 0;
    scriviCopia(false);
    return;
  }
  tabelleTx = s.r.flag & proto::FLAG_VERSIONE_TX;
  nTabelle = 0;
  for (const bande::Info& b : bande::ELENCO)
    if (bande::disponibile(b.banda, tabelleTx)) {
      tabelle[nTabelle].banda = b.banda;
      tabelle[nTabelle].n = 0;
      nTabelle++;
    }
  passo = Passo::LEGGI;
  iTabella = 0;
  attesa = false;
  imposta(Fase::IN_CORSO, "Lettura della calibrazione dall'antenna…");
}

// Lettura (salvataggio) o rilettura di controllo (ripristino) delle tabelle, una banda alla volta
void passoLettura(bool verifica) {
  TabellaCopia& t = tabelle[iTabella];
  if (!attesa) {
    if (link_antenna::occupato()) return;
    link_antenna::richiediTabella(t.banda);
    attesa = true;
    inizio = millis();
    imposta(Fase::IN_CORSO, "%s calibrazione: %s (%u di %u)", verifica ? "Controllo" : "Lettura", nomeBanda(t.banda),
            iTabella + 1, nTabelle);
    return;
  }
  link_antenna::Tabella r = link_antenna::tabella();
  if (r.banda == t.banda && r.completa) {
    if (verifica) {
      bool uguale = r.totale == t.n;
      for (uint8_t j = 0; uguale && j < t.n; j++) {
        bool trovato = false;  // stessa frequenza (entro 1 kHz) e stessa posizione
        for (uint8_t k = 0; k < r.totale; k++)
          trovato |= (uint32_t)abs((int32_t)(r.freq_hz[k] - t.f[j])) <= 1000 && r.posizione[k] == t.p[j];
        uguale = trovato;
      }
      if (!uguale) errori++;
    } else {
      t.n = r.totale;
      for (uint8_t j = 0; j < r.totale; j++) {
        t.f[j] = r.freq_hz[j];
        t.p[j] = r.posizione[j];
      }
    }
    attesa = false;
    if (++iTabella < nTabelle) return;
    passo = Passo::FERMO;
    if (!verifica) {
      scriviCopia(true);
    } else if (errori) {
      imposta(Fase::ERRORE, "Calibrazione sull'antenna diversa dalla copia in %u bande: firmware dell'antenna da "
              "aggiornare (CAL_SCRIVI)? Impostazioni e memorie ripristinate.", errori);
    } else {
      imposta(Fase::FINITA, "Ripristino completato (%u punti di calibrazione controllati): il pannello si riavvia",
              punti);
      riavvioAlle = millis() + RIAVVIO_DOPO_MS;
    }
    return;
  }
  if (millis() - inizio > ATTESA_TABELLA_MS) {
    passo = Passo::FERMO;
    imposta(Fase::ERRORE, "L'antenna non risponde: tabella dei %s non letta", nomeBanda(t.banda));
  }
}

// --- Ripristino ------------------------------------------------------------------------------

struct MemoriaCopia {
  char nome[sizeof(memorie::Memoria::nome)];
  uint32_t f;
};

void copiaTesto(char* dst, size_t n, const String& v) {
  strncpy(dst, v.c_str(), n - 1);
  dst[n - 1] = 0;
}

void avviaRipristino() {
  // Lettura del file
  String testo;
  bool montata = false, letto = false;
  if (lvgl_port_lock(-1)) {
    montata = monta();
    if (montata) {
      File f = SD.open(FILE_COPIA, FILE_READ);
      if (f) {
        testo = f.readString();
        f.close();
        letto = true;
      }
      smonta();
    }
    lvgl_port_unlock();
  }
  if (!montata) {
    imposta(Fase::ERRORE, "microSD non trovata: inseriscila e riprova");
    return;
  }
  if (!letto) {
    imposta(Fase::ERRORE, "%s non trovato sulla microSD", FILE_COPIA);
    return;
  }

  // Analisi: si parte dalle impostazioni attuali e si cambia solo quello che c'è nel file
  Impostazioni nuove = impostazioni::get();
  char qrzUtente[sizeof(ImpostazioniQrz::utente)];
  copiaTesto(qrzUtente, sizeof(qrzUtente), impostazioni::qrz().utente);
  uint32_t ultime[bande::NUMERO] = {0}, usata = 0;
  static MemoriaCopia mem[memorie::MAX_PERSONALI];
  uint8_t nMem = 0;
  bool memorieNelFile = false, conTabelle = false, formatoOk = false;
  nTabelle = 0;
  String sezione;
  int da = 0;
  while (da < (int)testo.length()) {
    int a = testo.indexOf('\n', da);
    if (a < 0) a = testo.length();
    String r = testo.substring(da, a);
    da = a + 1;
    r.trim();
    if (!r.length() || r[0] == '#') continue;
    if (r[0] == '[') {
      sezione = r;
      if (sezione == "[memorie]") memorieNelFile = true;
      continue;
    }
    if (sezione == "[memorie]") {
      int u = r.lastIndexOf('=');
      if (u <= 0 || nMem >= memorie::MAX_PERSONALI) continue;
      copiaTesto(mem[nMem].nome, sizeof(mem[nMem].nome), r.substring(0, u));
      mem[nMem].f = strtoul(r.substring(u + 1).c_str(), nullptr, 10);
      nMem++;
      continue;
    }
    if (sezione == "[calibrazione]") {
      if (r.startsWith("banda=")) {
        bande::Banda b = (bande::Banda)r.substring(6).toInt();
        if (bande::indice(b) < 0 || nTabelle >= bande::NUMERO) continue;
        tabelle[nTabelle].banda = b;
        tabelle[nTabelle].n = 0;
        nTabelle++;
      } else if (nTabelle) {
        TabellaCopia& t = tabelle[nTabelle - 1];
        int sp = r.indexOf(' ');
        if (sp > 0 && t.n < link_antenna::MAX_PUNTI) {
          t.f[t.n] = strtoul(r.substring(0, sp).c_str(), nullptr, 10);
          t.p[t.n] = (int32_t)strtol(r.substring(sp + 1).c_str(), nullptr, 10);
          t.n++;
        }
      }
      continue;
    }
    int u = r.indexOf('=');
    if (u <= 0) continue;
    String k = r.substring(0, u), v = r.substring(u + 1);
    uint32_t n = strtoul(v.c_str(), nullptr, 10);
    if (sezione == "") {
      if (k == "formato") formatoOk = v.toInt() == FORMATO;
      else if (k == "antenna") {
        conTabelle = true;
        tabelleTx = v == "tx";
      }
    } else if (sezione == "[impostazioni]") {
      if (k == "nominativo") copiaTesto(nuove.nominativo, sizeof(nuove.nominativo), v);
      else if (k == "locator") copiaTesto(nuove.locator, sizeof(nuove.locator), v);
      else if (k == "tema") nuove.tema = (uint8_t)n;
      else if (k == "screensaver") nuove.screensaver = (uint8_t)n;
      else if (k == "attesa_min") nuove.attesa_min = (uint8_t)n;
      else if (k == "luminosita") nuove.luminosita = (uint8_t)constrain(n, 10, 100);
      else if (k == "modo_ora") nuove.modoOra = (uint8_t)n;
      else if (k == "fuso") nuove.fuso = (uint8_t)n;
      else if (k == "wifi_rete") {
        if (v != nuove.wifiSsid) nuove.wifiPassword[0] = 0;  // altra rete: la password vecchia non serve
        copiaTesto(nuove.wifiSsid, sizeof(nuove.wifiSsid), v);
      } else if (k == "dec_modo") nuove.decModo = (uint8_t)n;
      else if (k == "cw_tono") nuove.cwTono = (uint16_t)n;
      else if (k == "cw_filtro") nuove.cwFiltro = (uint16_t)n;
      else if (k == "cw_wpm") nuove.cwWpm = (uint8_t)n;
      else if (k == "rtty_baud") nuove.rttyBaud = (uint8_t)n;
      else if (k == "rtty_shift") nuove.rttyShift = (uint16_t)n;
      else if (k == "rtty_mark") nuove.rttyMark = (uint16_t)n;
      else if (k == "rtty_stop") nuove.rttyStop = (uint8_t)n;
      else if (k == "rtty_opzioni") nuove.rttyOpzioni = (uint8_t)n;
      else if (k == "qrz_utente") copiaTesto(qrzUtente, sizeof(qrzUtente), v);
    } else if (sezione == "[ultime]") {
      if (k == "usata") usata = n;
      else {
        int i = bande::indice((bande::Banda)k.toInt());
        if (i >= 0) ultime[i] = n;
      }
    }
  }
  if (!formatoOk) {
    imposta(Fase::ERRORE, "%s non è una copia di loopant (o ha un formato diverso)", FILE_COPIA);
    return;
  }

  // Pannello: impostazioni, ultime frequenze, memorie personali
  if (lvgl_port_lock(-1)) {
    Impostazioni& imp = impostazioni::get();
    nuove.versione = imp.versione;
    memcpy(nuove.peer, imp.peer, sizeof(nuove.peer));  // l'associazione con l'antenna resta quella attuale
    imp = nuove;
    impostazioni::salva();
    copiaTesto(impostazioni::qrz().utente, sizeof(impostazioni::qrz().utente), qrzUtente);
    impostazioni::salvaQrz();
    for (size_t i = 0; i < bande::NUMERO; i++)
      if (ultime[i]) impostazioni::ricorda(ultime[i]);
    if (usata) impostazioni::ricorda(usata);
    if (memorieNelFile) {
      while (memorie::numeroPersonali()) memorie::elimina(memorie::numeroPersonali() - 1);
      for (uint8_t i = 0; i < nMem; i++) memorie::aggiungi(mem[i].nome, mem[i].f);
    }
    lvgl_port_unlock();
  }

  // Antenna: tabelle di calibrazione
  link_antenna::Stato s = link_antenna::stato();
  if (!conTabelle) {
    imposta(Fase::FINITA, "Impostazioni e memorie ripristinate (la copia non ha la calibrazione): il pannello si "
            "riavvia");
    riavvioAlle = millis() + RIAVVIO_DOPO_MS;
    return;
  }
  if (!s.collegato) {
    imposta(Fase::ERRORE, "Impostazioni e memorie ripristinate. Calibrazione NON ripristinata: antenna non "
            "collegata (ripeti il ripristino con l'antenna accesa)");
    return;
  }
  if ((bool)(s.r.flag & proto::FLAG_VERSIONE_TX) != tabelleTx) {
    imposta(Fase::ERRORE, "Impostazioni e memorie ripristinate. Calibrazione NON ripristinata: la copia è "
            "dell'antenna %s, quella collegata è %s", tabelleTx ? "TX" : "RX",
            (s.r.flag & proto::FLAG_VERSIONE_TX) ? "TX" : "RX");
    return;
  }
  passo = Passo::SCRIVI;
  iTabella = 0;
  iPunto = -1;
  attesa = false;
  punti = errori = 0;
  imposta(Fase::IN_CORSO, "Scrittura della calibrazione sull'antenna…");
}

// Per ogni banda: CAL_CANCELLA, poi un CAL_SCRIVI per punto, un comando alla volta
void passoScrittura() {
  if (attesa) {
    if (link_antenna::occupato()) {
      if (millis() - inizio > ATTESA_COMANDO_MS) {
        passo = Passo::FERMO;
        imposta(Fase::ERRORE, "L'antenna non risponde: calibrazione ripristinata solo in parte (%u punti)", punti);
      }
      return;
    }
    link_antenna::Stato s = link_antenna::stato();
    if (s.comandoFallito || (attesaScrivi && s.r.errore == proto::Errore::COMANDO_NON_VALIDO)) errori++;
    attesa = false;
  }
  if (iTabella >= nTabelle) {  // fine: rilettura di controllo
    passo = Passo::VERIFICA;
    iTabella = 0;
    attesa = false;
    if (errori) {
      passo = Passo::FERMO;
      imposta(Fase::ERRORE, "L'antenna ha rifiutato %u comandi: calibrazione ripristinata solo in parte", errori);
    }
    return;
  }
  TabellaCopia& t = tabelle[iTabella];
  const bande::Info* b = bande::info(t.banda);
  if (iPunto < 0) {
    link_antenna::invia(proto::Tipo::CAL_CANCELLA, b->min_hz);
    attesaScrivi = false;
    iPunto = 0;
  } else if (iPunto < t.n) {
    link_antenna::invia(proto::Tipo::CAL_SCRIVI, t.f[iPunto], t.p[iPunto]);
    attesaScrivi = true;
    iPunto++;
    punti++;
  } else {
    iTabella++;
    iPunto = -1;
    return;
  }
  attesa = true;
  inizio = millis();
  imposta(Fase::IN_CORSO, "Scrittura calibrazione: %s (%u di %u)", b->nome, iTabella + 1, nTabelle);
}

}  // namespace

bool salva() { return richiedi(Richiesta::SALVA); }
bool ripristina() { return richiedi(Richiesta::RIPRISTINA); }

void aggiorna() {
  portENTER_CRITICAL(&mux);
  Richiesta r = richiesta;
  richiesta = Richiesta::NESSUNA;
  if (r != Richiesta::NESSUNA) st.fase = Fase::IN_CORSO;
  portEXIT_CRITICAL(&mux);
  if (r == Richiesta::SALVA) avviaSalvataggio();
  else if (r == Richiesta::RIPRISTINA) avviaRipristino();

  switch (passo) {
    case Passo::LEGGI: passoLettura(false); break;
    case Passo::VERIFICA: passoLettura(true); break;
    case Passo::SCRIVI: passoScrittura(); break;
    case Passo::FERMO: break;
  }
  if (riavvioAlle && (int32_t)(millis() - riavvioAlle) >= 0) ESP.restart();
}

Stato stato() {
  portENTER_CRITICAL(&mux);
  Stato s = st;
  portEXIT_CRITICAL(&mux);
  return s;
}

}  // namespace copia_sd
