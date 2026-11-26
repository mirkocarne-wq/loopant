#include "server_web.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <bande.h>
#include <string.h>

#include "impostazioni.h"
#include "link_antenna.h"
#include "pagina_web.h"
#include "rete.h"
#include "tocco_remoto.h"
#include "waveshare/rgb_lcd_port.h"

namespace server_web {
namespace {

WebServer server(80);
volatile bool avviato = false;
char ip[20] = "";

constexpr int LARGHEZZA = 1024, ALTEZZA = 600;

// Testo JSON con virgolette e barre protette
String json(const char* s) {
  String o = "\"";
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') o += '\\';
    if ((unsigned char)*s >= 0x20) o += *s;
  }
  return o + "\"";
}

void rispondiJson(int codice, const String& corpo) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(codice, "application/json; charset=utf-8", corpo);
}

void pagina() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGINA_WEB);
}

void stato() {
  link_antenna::Stato s = link_antenna::stato();
  const Impostazioni& imp = impostazioni::get();
  char r[96];
  rete::stato(r, sizeof(r));
  String o = "{";
  o += "\"nominativo\":" + json(imp.nominativo);
  o += ",\"locator\":" + json(imp.locator);
  o += ",\"collegato\":" + String(s.collegato ? "true" : "false");
  o += ",\"peer\":" + String(s.peerImpostato ? "true" : "false");
  o += ",\"stato\":" + json(proto::nomeStato(s.r.stato));
  o += ",\"errore\":" + json(proto::nomeErrore(s.r.errore));
  o += ",\"freq_hz\":" + String((unsigned long)s.r.freq_hz);
  o += ",\"posizione\":" + String((long)s.r.posizione);
  o += ",\"azzerata\":" + String((s.r.flag & proto::FLAG_AZZERATA) ? "true" : "false");
  // Bande dell'antenna associata con l'ultima frequenza usata (pulsanti della pagina)
  o += ",\"ultima\":" + String((unsigned long)impostazioni::ultimaUsata());
  o += ",\"bande\":[";
  bool primo = true;
  for (const bande::Info& b : bande::ELENCO) {
    if (!bande::disponibile(b.banda, impostazioni::antennaTx())) continue;
    o += String(primo ? "" : ",") + "{\"nome\":" + json(b.nome) + ",\"hz\":" +
         String((unsigned long)impostazioni::ultima(b.banda)) + "}";
    primo = false;
  }
  o += "]";
  o += ",\"rete\":" + json(r);
  o += "}";
  rispondiJson(200, o);
}

void comando() {
  String azione = server.arg("azione");
  if (azione == "stop") {
    link_antenna::invia(proto::Tipo::STOP);
  } else if (azione == "zero") {
    link_antenna::invia(proto::Tipo::ZERO);
  } else if (azione == "goto") {
    String k = server.arg("khz");
    k.replace(",", ".");
    uint32_t hz = (uint32_t)(k.toFloat() * 1000.0f + 0.5f);
    if (bande::di(hz, impostazioni::antennaTx()) == bande::Banda::NESSUNA) {
      String e = "Frequenza fuori banda:";
      bool primo = true;
      for (const bande::Info& b : bande::ELENCO)
        if (bande::disponibile(b.banda, impostazioni::antennaTx())) {
          e += String(primo ? " " : ", ") + String(b.min_hz / 1000.0, 1) + "-" + String(b.max_hz / 1000.0, 1);
          primo = false;
        }
      rispondiJson(400, "{\"ok\":false,\"errore\":" + json(e.c_str()) + "}");
      return;
    }
    if (!link_antenna::stato().collegato) {
      rispondiJson(409, "{\"ok\":false,\"errore\":\"Antenna non collegata\"}");
      return;
    }
    link_antenna::invia(proto::Tipo::GOTO, hz);
    impostazioni::ricorda(hz);
  } else {
    rispondiJson(400, "{\"ok\":false,\"errore\":\"Comando sconosciuto\"}");
    return;
  }
  rispondiJson(200, "{\"ok\":true}");
}

void tocco() {
  if (!server.hasArg("x") || !server.hasArg("y")) {
    rispondiJson(400, "{\"ok\":false}");
    return;
  }
  tocco_remoto::tocca((int16_t)server.arg("x").toInt(), (int16_t)server.arg("y").toInt());
  rispondiJson(200, "{\"ok\":true}");
}

void scrivi16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
void scrivi32(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }

// Schermo come BMP a 16 bit (RGB565, righe dall'alto): copia del framebuffer.
// ?scala=2: metà risoluzione (512x300, un quarto dei dati) per l'aggiornamento continuo.
void schermo() {
  const int scala = server.arg("scala") == "2" ? 2 : 1;
  const int L = LARGHEZZA / scala, A = ALTEZZA / scala;
  void *fb1 = nullptr, *fb2 = nullptr;
  waveshare_get_frame_buffer(&fb1, &fb2);
  if (!fb1) {
    server.send(503, "text/plain", "Framebuffer non disponibile");
    return;
  }
  const uint32_t dati = (uint32_t)L * A * 2, intestazione = 14 + 40 + 12;
  uint8_t h[intestazione] = {};
  h[0] = 'B';
  h[1] = 'M';
  scrivi32(h + 2, intestazione + dati);
  scrivi32(h + 10, intestazione);
  scrivi32(h + 14, 40);
  scrivi32(h + 18, L);
  scrivi32(h + 22, (uint32_t)(-A));  // altezza negativa: righe dall'alto
  scrivi16(h + 26, 1);
  scrivi16(h + 28, 16);
  scrivi32(h + 30, 3);  // BI_BITFIELDS
  scrivi32(h + 34, dati);
  scrivi32(h + 54, 0xF800);  // maschere RGB565
  scrivi32(h + 58, 0x07E0);
  scrivi32(h + 62, 0x001F);
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(intestazione + dati);
  server.send(200, "image/bmp", "");
  WiFiClient c = server.client();
  c.write(h, intestazione);
  const uint16_t* fb = (const uint16_t*)fb1;
  if (scala == 1) {
    const uint8_t* p = (const uint8_t*)fb;
    for (uint32_t inviati = 0; inviati < dati && c.connected();) {
      size_t n = dati - inviati > 8192 ? 8192 : dati - inviati;
      size_t w = c.write(p + inviati, n);
      if (!w) break;
      inviati += w;
    }
    return;
  }
  static uint16_t riga[LARGHEZZA / 2 * 8];  // 8 righe ridotte per invio
  for (int y = 0; y < A && c.connected(); y += 8) {
    int righe = A - y < 8 ? A - y : 8;
    for (int r = 0; r < righe; r++) {
      const uint16_t* src = fb + (size_t)(y + r) * scala * LARGHEZZA;
      for (int x = 0; x < L; x++) riga[r * L + x] = src[x * scala];
    }
    size_t n = (size_t)righe * L * 2;
    const uint8_t* p = (const uint8_t*)riga;
    for (size_t inviati = 0; inviati < n;) {
      size_t w = c.write(p + inviati, n - inviati);
      if (!w) return;
      inviati += w;
    }
  }
}

void nonTrovato() { server.send(404, "text/plain", "Pagina non trovata"); }

void compito(void*) {
  for (;;) {
    if (!avviato) {
      if (WiFi.status() == WL_CONNECTED) {
        server.on("/", HTTP_GET, pagina);
        server.on("/stato", HTTP_GET, stato);
        server.on("/comando", HTTP_POST, comando);
        server.on("/tocco", HTTP_POST, tocco);
        server.on("/schermo.bmp", HTTP_GET, schermo);
        server.onNotFound(nonTrovato);
        server.begin();
        if (MDNS.begin("loopant")) MDNS.addService("http", "tcp", 80);
        snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
        avviato = true;
      } else {
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }
    static uint32_t ultimoIp = 0;
    if (millis() - ultimoIp > 2000) {  // l'indirizzo può cambiare (DHCP, riconnessione)
      ultimoIp = millis();
      if (WiFi.status() == WL_CONNECTED) snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
      else ip[0] = 0;
    }
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(3));
  }
}

}  // namespace

void inizia() { xTaskCreatePinnedToCore(compito, "web", 8192, nullptr, 1, nullptr, 0); }

bool attivo() { return avviato && ip[0]; }

void indirizzo(char* out, size_t n) {
  if (attivo()) snprintf(out, n, "http://%s · http://loopant.local", ip);
  else if (n) out[0] = 0;
}

}  // namespace server_web
