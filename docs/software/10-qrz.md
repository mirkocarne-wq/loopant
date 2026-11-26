# 10 — QRZ.com (pannello)

Scheda **QRZ** del pannello: ricerca dei nominativi e "I miei QSO". Richiesta dell'utente del 21/11/2026,
da realizzare "con e senza abbonamento".
Legenda: **[D]** = deciso; **[N]** = proposta tecnica, da confermare.

## Servizi di QRZ (documentazione ufficiale, letta il 21/11/2026)
| Servizio | Indirizzo | Accesso | Senza abbonamento | Con abbonamento «XML Logbook Data» |
|---|---|---|---|---|
| **XML Data** (nominativi) | `https://xmldata.qrz.com/xml/current/` | utente e password QRZ → chiave di sessione | si accede, ma con **campi limitati** ("per prove") | dati completi |
| **Logbook API** (QSO) | `https://logbook.qrz.com/api` | chiave API del logbook (sito QRZ → Logbook → Settings) | **no**: FETCH e STATUS richiedono l'abbonamento | sì |

- La Logbook API vuole un **user agent identificabile**: il pannello usa `loopant/0.1 (pannello mag loop, ESP32-S3)`.
- Con una chiave errata QRZ risponde **HTTP 401** con `RESULT=AUTH` e `REASON=… invalid api key …` (provato).
  `RESULT=AUTH` vale anche per permessi insufficienti: il pannello distingue dal testo del motivo.
- Il formato dell'ADIF nella risposta di FETCH (entità HTML `&lt;` o codifica URL `%3C`) non è documentato:
  il pannello accetta entrambi. **Da verificare con un account abbonato.**

## Requisiti
| ID | Requisito | |
|---|---|---|
| Q1 | Ricerca nominativo: nome, QTH, paese/DXCC, locator, zone CQ/ITU, QSL (LoTW, eQSL, carta, manager) | [D] |
| Q2 | Distanza e direzione dalla propria stazione (percorso corto e lungo), calcolate sul pannello | [D] |
| Q3 | Mappa con la propria stazione, la stazione cercata e il percorso sul cerchio massimo, sopra notte e grey line | [D] |
| Q4 | Ricerche recenti (ultime 5, in memoria fino allo spegnimento) | [D] |
| Q5 | I miei QSO: ultimi 40, filtri Tutti e una per banda dell'antenna, totale e confermati; tocco su un QSO → ricerca del nominativo; **Accorda** sulla frequenza del QSO (solo nelle bande dell'antenna) | [D] |
| Q6 | Senza abbonamento: la ricerca funziona con i dati disponibili; il logbook mostra perché non è disponibile | [D] |
| Q7 | Credenziali in Impostazioni (utente, password, chiave API), «Prova accesso» | [D] |
| Q8 | Foto della stazione cercata (JPEG da QRZ) | [N] più avanti |

## Realizzazione
Codice: `firmware/pannello/src/qrz.*` (lettura delle risposte, C++ puro), `geo.*` (distanza, direzione, percorso),
`servizio_qrz.*` (HTTPS in un task sul core 0), `ui/pagina_qrz.cpp`, `mappa.*` (stazione cercata e percorso).

- **HTTPS con verifica del certificato**: pacchetto dei certificati radice incluso nel core ESP32 3.1.1
  (`_binary_x509_crt_bundle_start`), passato a `NetworkClientSecure::setCACertBundle`.
- **Sessione XML**: un accesso, poi la chiave viene riusata; se scade, nuovo accesso e secondo tentativo.
  Utente e password vanno in POST, non nell'indirizzo.
- **Ultimi QSO** con tre richieste:
  1. `OPTION=TYPE:LOGIDS[,BAND:40m]`: elenco leggero dei logid e totale;
  2. `OPTION=LOGIDS:a+b+…`: l'ADIF dei 40 logid più alti;
  3. `OPTION=STATUS:CONFIRMED,MAX:0[,BAND:…]`: solo il numero dei confermati.
  I QSO si ordinano per data e ora.
- **Credenziali** in NVS in un blocco separato (`impostazioni::qrz()`), in chiaro come la password del WiFi.
- **Prove sul PC** (`test/test_qrz.cpp`): risposte XML con e senza abbonamento, non trovato, sessione scaduta,
  logbook con entità e con codifica URL, RESULT=AUTH, risposta vera con chiave errata, logid, codifica URL,
  distanza/direzione Londra–New York (5570 km, 288°).

## Memoria (problema trovato e risolto il 21/11/2026)
La prima richiesta HTTPS mandava in crash il pannello: la RAM interna libera era di soli **820 byte**.
Con la configurazione del core le allocazioni sotto i 4 KB vanno in RAM interna e LVGL vi metteva migliaia di oggetti.
Ora LVGL alloca in **PSRAM** (`lv_conf.h`): RAM interna libera circa 137 KB, minimo circa 84 KB durante l'HTTPS.
Il comando `mem` della console mostra la situazione.
