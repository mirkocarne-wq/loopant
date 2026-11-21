#include "rete.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <string.h>

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

void compito(void*) {
  uint32_t ultimiIndici = 0;
  bool primiIndici = true;
  for (;;) {
    const Impostazioni& imp = impostazioni::get();
    if (daRiavviare) {
      daRiavviare = false;
      WiFi.disconnect(false, false);
      connessa = false;
    }
    if (!imp.wifiSsid[0]) {
      connessa = false;
      scriviStato("WiFi non configurato");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (WiFi.status() != WL_CONNECTED) {
      connessa = false;
      char t[96];
      snprintf(t, sizeof(t), "Connessione a %s…", imp.wifiSsid);
      scriviStato(t);
      WiFi.begin(imp.wifiSsid, imp.wifiPassword);
      uint32_t inizio = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - inizio < ATTESA_CONNESSIONE_MS && !daRiavviare)
        vTaskDelay(pdMS_TO_TICKS(250));
      if (WiFi.status() != WL_CONNECTED) {
        snprintf(t, sizeof(t), "Rete %s non raggiungibile: nuovo tentativo tra 30 s", imp.wifiSsid);
        scriviStato(t);
        for (int i = 0; i < 120 && !daRiavviare; i++) vTaskDelay(pdMS_TO_TICKS(250));
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
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

}  // namespace

void inizia() {
  WiFi.setAutoReconnect(true);
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

}  // namespace rete
