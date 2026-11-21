#include "rete.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"
#include "impostazioni.h"
#include "indici.h"
#include "ora.h"

namespace rete {
namespace {

// Feed degli indici solari (formato da confermare sul feed reale; condizioni d'uso da verificare).
const char* URL_INDICI = "http://www.hamqsl.com/solarxml.php";
constexpr uint32_t INTERVALLO_INDICI_MS = 2UL * 60 * 60 * 1000;  // 2 ore
constexpr uint32_t ATTESA_CONNESSIONE_MS = 15000;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
char testoStato[96] = "WiFi non configurato";
volatile bool daRiavviare = false;
volatile bool indiciSubito = false;
volatile bool connessa = false;

volatile bool scansioneRichiesta = false;
volatile bool scansioneInCorso = false;
Rete risultati[MAX_RETI];
uint8_t nRisultati = 0;
uint32_t versioneScansione = 0;
int16_t codiceScansione = 0;
uint32_t durataScansione = 0;

// Interrompe le attese del task quando serve fare qualcosa subito
bool interrompere() { return daRiavviare || scansioneRichiesta; }

void scriviStato(const char* t) {
  portENTER_CRITICAL(&mux);
  strncpy(testoStato, t, sizeof(testoStato) - 1);
  testoStato[sizeof(testoStato) - 1] = 0;
  portEXIT_CRITICAL(&mux);
}

void scaricaIndici() {
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(URL_INDICI)) {
    indici::errore("Indirizzo del feed non valido");
    return;
  }
  int codice = http.GET();
  if (codice != 200) {
    char e[48];
    snprintf(e, sizeof(e), "Feed indici: errore HTTP %d", codice);
    indici::errore(e);
    http.end();
    return;
  }
  String corpo = http.getString();
  http.end();
  indici::Dati d;
  if (indici::analizza(corpo.c_str(), d)) {
    d.aggiornato = time(nullptr);
    indici::imposta(d);
  } else {
    indici::errore("Feed indici: formato non riconosciuto");
  }
}

void eseguiScansione() {
  scansioneInCorso = true;
  bool eraCollegata = WiFi.status() == WL_CONNECTED;
  // Mentre la stazione prova a collegarsi (rete salvata assente, per esempio fuori casa) l'ESP32 rifiuta la ricerca
  // ("sta is connecting"): si ferma il tentativo e, se serve, si riprova qualche volta.
  uint32_t inizio = millis();
  int16_t n = WIFI_SCAN_FAILED;
  for (int tentativo = 0; tentativo < 6 && n < 0; tentativo++) {
    if (!eraCollegata) WiFi.disconnect(false, false);
    if (tentativo) vTaskDelay(pdMS_TO_TICKS(300));
    n = WiFi.scanNetworks(false, false);
  }
  codiceScansione = n;
  durataScansione = millis() - inizio;

  Rete tmp[MAX_RETI];
  uint8_t k = 0;
  for (int16_t i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;  // reti nascoste: si inseriscono a mano
    int8_t rssi = (int8_t)WiFi.RSSI(i);
    uint8_t j = 0;
    while (j < k && strcmp(tmp[j].ssid, ssid.c_str())) j++;
    if (j == k && k == MAX_RETI) {  // elenco pieno: si sostituisce la più debole
      uint8_t debole = 0;
      for (uint8_t q = 1; q < k; q++)
        if (tmp[q].rssi < tmp[debole].rssi) debole = q;
      if (rssi <= tmp[debole].rssi) continue;
      j = debole;
    } else if (j < k) {  // stessa rete su più ripetitori: si tiene il segnale più forte
      if (rssi <= tmp[j].rssi) continue;
    } else {
      k++;
    }
    strncpy(tmp[j].ssid, ssid.c_str(), sizeof(tmp[j].ssid) - 1);
    tmp[j].ssid[sizeof(tmp[j].ssid) - 1] = 0;
    tmp[j].rssi = rssi;
    tmp[j].protetta = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    tmp[j].canale = (uint8_t)WiFi.channel(i);
  }
  WiFi.scanDelete();
  for (uint8_t a = 1; a < k; a++) {  // dal segnale più forte
    Rete v = tmp[a];
    int b = a - 1;
    while (b >= 0 && tmp[b].rssi < v.rssi) {
      tmp[b + 1] = tmp[b];
      b--;
    }
    tmp[b + 1] = v;
  }
  // senza router la radio deve tornare sul canale dell'antenna
  if (WiFi.status() != WL_CONNECTED) esp_wifi_set_channel(cfg::CANALE_WIFI, WIFI_SECOND_CHAN_NONE);

  portENTER_CRITICAL(&mux);
  memcpy(risultati, tmp, sizeof(Rete) * k);
  nRisultati = k;
  versioneScansione++;
  portEXIT_CRITICAL(&mux);
  scansioneInCorso = false;
  if (n < 0) scriviStato("Ricerca delle reti non riuscita");
}

void compito(void*) {
  uint32_t ultimiIndici = 0;
  bool primiIndici = true;
  for (;;) {
    const Impostazioni& imp = impostazioni::get();
    if (scansioneRichiesta) {
      scansioneRichiesta = false;
      eseguiScansione();
    }
    if (daRiavviare) {
      daRiavviare = false;
      WiFi.disconnect(false, false);
      connessa = false;
    }
    if (!imp.wifiSsid[0]) {
      connessa = false;
      scriviStato("WiFi non configurato");
      for (int i = 0; i < 4 && !interrompere(); i++) vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    if (WiFi.status() != WL_CONNECTED) {
      connessa = false;
      char t[96];
      snprintf(t, sizeof(t), "Connessione a %s…", imp.wifiSsid);
      scriviStato(t);
      WiFi.begin(imp.wifiSsid, imp.wifiPassword);
      uint32_t inizio = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - inizio < ATTESA_CONNESSIONE_MS && !interrompere())
        vTaskDelay(pdMS_TO_TICKS(250));
      if (WiFi.status() != WL_CONNECTED) {
        if (interrompere()) continue;
        snprintf(t, sizeof(t), "Rete %s non raggiungibile: nuovo tentativo tra 30 s", imp.wifiSsid);
        scriviStato(t);
        for (int i = 0; i < 120 && !interrompere(); i++) vTaskDelay(pdMS_TO_TICKS(250));
        continue;
      }
      // NTP: l'ora arriva in background; poi si rimette il fuso scelto
      configTime(0, 0, "pool.ntp.org", "time.google.com");
      ora::applicaFuso();
      primiIndici = true;
    }
    if (!connessa) {
      connessa = true;
      char t[96];
      snprintf(t, sizeof(t), "Collegato a %s · canale %d · %s", imp.wifiSsid, WiFi.channel(),
               WiFi.localIP().toString().c_str());
      scriviStato(t);
    }
    bool tempo = millis() - ultimiIndici >= INTERVALLO_INDICI_MS;
    if ((primiIndici || tempo || indiciSubito) && ora::valida()) {
      primiIndici = false;
      indiciSubito = false;
      ultimiIndici = millis();
      scaricaIndici();
    }
    for (int i = 0; i < 4 && !interrompere(); i++) vTaskDelay(pdMS_TO_TICKS(250));
  }
}

}  // namespace

void inizia() {
  // La riconnessione la fa il task (tentativo di 15 s, poi pausa di 30 s). Quella automatica del core riprova
  // senza sosta quando la rete non c'è e in quei momenti la ricerca delle reti viene rifiutata.
  WiFi.setAutoReconnect(false);
  xTaskCreatePinnedToCore(compito, "rete", 8192, nullptr, 1, nullptr, 0);
}

void riavvia() { daRiavviare = true; }

bool collegata() { return connessa; }

void stato(char* out, size_t n) {
  portENTER_CRITICAL(&mux);
  strncpy(out, testoStato, n - 1);
  portEXIT_CRITICAL(&mux);
  out[n - 1] = 0;
}

void aggiornaIndiciOra() { indiciSubito = true; }

void collega(const char* ssid, const char* password) {
  Impostazioni& imp = impostazioni::get();
  strncpy(imp.wifiSsid, ssid ? ssid : "", sizeof(imp.wifiSsid) - 1);
  imp.wifiSsid[sizeof(imp.wifiSsid) - 1] = 0;
  strncpy(imp.wifiPassword, password ? password : "", sizeof(imp.wifiPassword) - 1);
  imp.wifiPassword[sizeof(imp.wifiPassword) - 1] = 0;
  impostazioni::salva();
  daRiavviare = true;
}

void avviaScansione() {
  if (!scansioneInCorso) {
    scansioneInCorso = true;  // subito, così l'interfaccia mostra "ricerca in corso"
    scansioneRichiesta = true;
  }
}

uint8_t reti(Rete* out, uint8_t max, bool& inCorso, uint32_t& versione) {
  portENTER_CRITICAL(&mux);
  uint8_t n = nRisultati < max ? nRisultati : max;
  memcpy(out, risultati, sizeof(Rete) * n);
  versione = versioneScansione;
  portEXIT_CRITICAL(&mux);
  inCorso = scansioneInCorso;
  return n;
}

void esitoScansione(int16_t& codice, uint32_t& durataMs, uint8_t& modo) {
  codice = codiceScansione;
  durataMs = durataScansione;
  modo = (uint8_t)WiFi.getMode();
}

}  // namespace rete
