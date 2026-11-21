// Simulatore sul PC: compila la vera interfaccia del pannello con LVGL 8.4 e salva le schermate in PPM.
// Collegamento, WiFi, audio e decoder sono finti, con dati di esempio.
// Uso: ./compila.sh <cartella lvgl 8.4> && ./simulatore <cartella di uscita>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <chrono>
#include <string>

#include "collegamento.h"
#include "decodifica.h"
#include "eventi.h"
#include "impostazioni.h"
#include "link_antenna.h"
#include "memorie.h"
#include "motore_antenna.h"
#include "ora.h"
#include "rete.h"
#include "schermo.h"
#include "tema.h"
#include "ui/pagine.h"
#include "ui/screensaver.h"
#include "ui/ui.h"

// --- Tempo virtuale -----------------------------------------------------------------------------
static uint32_t msVirtuali = 0;
uint32_t millis() { return msVirtuali; }

// --- Collegamento finto -------------------------------------------------------------------------
bool Collegamento::inizia(const uint8_t*, const uint8_t*, const uint8_t*, uint8_t) { return true; }
bool Collegamento::invia(const void*, size_t) { return true; }
bool Collegamento::ricevi(Pacchetto&) { return false; }
bool Collegamento::peerValido() const { return true; }
void Collegamento::macLocale(uint8_t out[6]) {
  const uint8_t m[6] = {0x24, 0x58, 0x7C, 0x00, 0x00, 0x01};
  memcpy(out, m, 6);
}
bool Collegamento::leggiMac(const char*, uint8_t*) { return false; }
void Collegamento::scriviMac(const uint8_t mac[6], char out[18]) {
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static link_antenna::Stato statoFinto;

namespace link_antenna {
void inizia(const uint8_t*) {}
void aggiorna() {}
void invia(proto::Tipo tipo, uint32_t freq, int32_t valore) {
  printf("  [comando all'antenna] tipo %d, freq %u, valore %d\n", (int)tipo, freq, valore);
}
Stato stato() {
  Stato s = statoFinto;
  s.etaMs = 400;
  return s;
}
void richiediTabella(bande::Banda) {}
Tabella tabella() {
  Tabella t = {};
  t.banda = bande::Banda::B40M;
  t.completa = true;
  t.totale = 3;
  const uint32_t f[3] = {7000000, 7100000, 7200000};
  const int32_t p[3] = {412800, 401200, 390100};
  for (int i = 0; i < 3; i++) {
    t.freq_hz[i] = f[i];
    t.posizione[i] = p[i];
  }
  t.versione = 1;
  return t;
}
}  // namespace link_antenna

// --- Rete, schermo e decoder finti ---------------------------------------------------------------
namespace rete {
void inizia() {}
void riavvia() {}
bool collegata() { return true; }
void stato(char* out, size_t n) { snprintf(out, n, "Collegato a [rete di casa] · canale 6 · 192.168.1.50"); }
void aggiornaIndiciOra() {}
void collega(const char* ssid, const char* password) {
  snprintf(impostazioni::get().wifiSsid, sizeof(Impostazioni::wifiSsid), "%s", ssid);
  snprintf(impostazioni::get().wifiPassword, sizeof(Impostazioni::wifiPassword), "%s", password);
}
static uint32_t versioneReti = 0;
void avviaScansione() { versioneReti++; }
uint8_t reti(Rete* out, uint8_t max, bool& inCorso, uint32_t& versione) {
  const Rete esempio[] = {{"[rete di casa]", -48, true, 6}, {"[rete di casa]_5G", -55, true, 36},
                          {"[vicino 1]", -67, true, 1},     {"[ospiti]", -71, false, 11},
                          {"[vicino 2]", -79, true, 6},     {"[vicino 3]", -86, true, 13}};
  uint8_t n = 0;
  for (; n < sizeof(esempio) / sizeof(esempio[0]) && n < max; n++) out[n] = esempio[n];
  inCorso = false;
  versione = versioneReti;
  return n;
}
}  // namespace rete

namespace schermo {
void accendi() {}
void spegni() {}
void luminosita(uint8_t) {}
}  // namespace schermo

namespace decodifica {
void inizia() {}
void configura() {}
Stato stato() {
  Stato s = {};
  s.ingresso = true;
  s.audio = 0.32f;
  s.rtty = impostazioni::get().decModo == 1;
  s.cwLivello = 0.68f;
  s.cwSoglia = 0.42f;
  s.cwWpm = 18.2f;
  strcpy(s.cwElementi, "-.-");
  s.rttyMark = 0.82f;
  s.rttySpace = 0.77f;
  s.rttyAfc = 3.4f;
  s.rttyAgganciato = true;
  return s;
}
uint32_t testo(char* out, size_t n) {
  const char* t = impostazioni::get().decModo == 1
                      ? "RYRYRYRYRYRY\nCQ CQ CQ DE [CALL] [CALL] [CALL]\nCQ CQ CQ DE [CALL] [CALL] [CALL] PSE K\n"
                        "[CALL] DE [CALL] UR 599 599 "
                      : "CQ CQ CQ DE [CALL] [CALL] [CALL] PSE K\n[CALL] DE [CALL] GM UR RST 599 599 NAME [NOME] "
                        "QTH [QTH] HW? BK\nR R TNX FER QSO 73 ";
  snprintf(out, n, "%s", t);
  return impostazioni::get().decModo;
}
void pulisci() {}
}  // namespace decodifica

// --- Schermo virtuale ----------------------------------------------------------------------------
static lv_color_t fb[1024 * 600];

static void flush(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* px) {
  for (int y = a->y1; y <= a->y2; y++)
    memcpy(&fb[y * 1024 + a->x1], px + (y - a->y1) * (a->x2 - a->x1 + 1), (a->x2 - a->x1 + 1) * sizeof(lv_color_t));
  lv_disp_flush_ready(drv);
}

static void avanza(int ms) {
  for (int t = 0; t < ms; t += 10) {
    msVirtuali += 10;
    lv_tick_inc(10);
    lv_timer_handler();
  }
}

static std::string uscita;

static void salva(const char* nome) {
  avanza(600);
  lv_refr_now(nullptr);
  std::string p = uscita + "/" + nome + ".ppm";
  FILE* f = fopen(p.c_str(), "wb");
  fprintf(f, "P6\n1024 600\n255\n");
  for (int i = 0; i < 1024 * 600; i++) {
    uint16_t c = fb[i].full;
    uint8_t rgb[3] = {(uint8_t)(((c >> 11) & 0x1F) * 255 / 31), (uint8_t)(((c >> 5) & 0x3F) * 255 / 63),
                      (uint8_t)((c & 0x1F) * 255 / 31)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
  printf("%s\n", p.c_str());
}

static void eventiDiEsempio() {
  link_antenna::Stato s = statoFinto;
  eventi::osserva(s);
  s.r.stato = proto::Stato::AZZERAMENTO;
  eventi::osserva(s);
  s.r.stato = proto::Stato::IN_MOVIMENTO;
  eventi::osserva(s);
  s.r.stato = proto::Stato::FERMO;
  eventi::osserva(s);
}

int main(int argc, char** argv) {
  uscita = argc > 1 ? argv[1] : "schermate";
  mkdir(uscita.c_str(), 0755);

  statoFinto = {};
  statoFinto.collegato = statoFinto.ricevuta = statoFinto.peerImpostato = true;
  statoFinto.r.versione = proto::VERSIONE;
  statoFinto.r.stato = proto::Stato::FERMO;
  statoFinto.r.posizione = 214380;
  statoFinto.r.freq_hz = 7074000;
  statoFinto.r.banda = 40;
  statoFinto.r.flag = proto::FLAG_VERSIONE_TX | proto::FLAG_AZZERATA;
  statoFinto.rssi = -58;
  statoFinto.inviati = 1240;
  statoFinto.ripetuti = 3;
  statoFinto.ricevuti = 1237;

  lv_init();
  static lv_disp_draw_buf_t db;
  static lv_color_t buf[1024 * 60];
  lv_disp_draw_buf_init(&db, buf, nullptr, 1024 * 60);
  static lv_disp_drv_t drv;
  lv_disp_drv_init(&drv);
  drv.hor_res = 1024;
  drv.ver_res = 600;
  drv.flush_cb = flush;
  drv.draw_buf = &db;
  lv_disp_drv_register(&drv);

  impostazioni::carica();
  Impostazioni& imp = impostazioni::get();
  imp.nominativo[0] = 0;  // compare il segnaposto
  strcpy(imp.locator, "JN61fv");
  strcpy(imp.wifiSsid, "[rete di casa]");
  imp.peer[0] = 0x24;
  imp.peer[5] = 0x02;
  memorie::carica();
  memorie::aggiungi("Net del martedì", 7150000);
  ora::applicaFuso();
  tema::inizia();
  eventiDiEsempio();
  ui::inizia();

  const char* nomi[] = {"sintonia", "memorie", "propagazione", "decoder", "diagnostica", "impostazioni", "calibrazione"};
  const char* temi[] = {"notte", "giorno", "fosfori-verdi", "fosfori-ambra"};
  for (uint8_t tm = 0; tm < tema::NUMERO; tm++) {
    imp.tema = tm;
    ui::richiediRicostruzione();
    avanza(300);
    for (uint8_t p = 0; p < ui::NUM_PAGINE; p++) {
      if (tm > 0 && p != ui::SINTONIA && p != ui::PROPAGAZIONE && p != ui::DECODER) continue;
      imp.decModo = 0;
      ui::vaiA(p);
      salva((std::string(temi[tm]) + "-" + std::to_string(p + 1) + "-" + nomi[p]).c_str());
      if (p == ui::DECODER) {
        imp.decModo = 1;
        ui::richiediRicostruzione();
        avanza(300);
        ui::vaiA(p);
        salva((std::string(temi[tm]) + "-" + std::to_string(p + 1) + "-decoder-rtty").c_str());
        imp.decModo = 0;
        ui::richiediRicostruzione();
        avanza(300);
      }
    }
    if (tm == 0) {  // sezione Antenna con l'antenna RX e il motore via CAN
      uint8_t flag = statoFinto.r.flag;
      statoFinto.r.flag = proto::FLAG_CAN | proto::FLAG_AZZERATA;
      motore_antenna::scelte().modo = 1;
      ui::richiediRicostruzione();
      avanza(300);
      ui::vaiA(ui::IMPOSTAZIONI);
      pag_impostazioni::mostraAntenna();
      salva("notte-6d-impostazioni-motore");
      statoFinto.r.flag = flag;
      motore_antenna::scelte().modo = 0;
      ui::richiediRicostruzione();
      avanza(300);
    }
    if (tm == 0) {  // finestra "Reti WiFi" e password
      ui::vaiA(ui::IMPOSTAZIONI);
      pag_impostazioni::mostraReti();
      salva("notte-6b-impostazioni-reti");
      pag_impostazioni::scegliRete(0);
      salva("notte-6c-impostazioni-password");
    }
    const char* stili[] = {"orologio", "frequenza", "greyline"};
    for (uint8_t s = 0; s < 3; s++) {
      if (tm != 0 && !(tm == 2 && s == 2) && !(tm == 3 && s == 1)) continue;
      imp.screensaver = s;
      screensaver::anteprima();
      salva((std::string(temi[tm]) + "-screensaver-" + stili[s]).c_str());
      screensaver::chiudi();
      avanza(100);
    }
  }
  return 0;
}
