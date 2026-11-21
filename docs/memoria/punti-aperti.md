# Punti aperti

Da chiudere. Quando uno è risolto, spostare il risultato nel file di memoria giusto e toglierlo da qui.

## Misure
- [ ] Variabile 1000 pF: capacità minima e massima; continuità albero–telaio.
- [ ] Diametro dell'albero del variabile (giunto 8→6 mm?).
- [ ] Riduttore: foro di ingresso da 5 mm (albero SERVO42D) e albero di uscita (8 mm?).
- [ ] Tubi di rame/ottone e PTFE: disponibilità e misure reali con il calibro (diametro esterno e parete).
- [ ] Modulo NEMA11: corrente nominale, finecorsa inclusi?, giunti.
- [ ] Rumore dell'alimentatore da 24 V sui 40 e 20 m.
- [ ] Efficienza reale e Q dell'anello (NanoVNA).

## Progetto
- [ ] Disegno quotato della scatola RX e del gruppo TX.

## Strumenti
- [x] Avviso malware al primo `pio run` (21/11/2026): causa non trovata nei log; l'antenna ora si compila con
  arduino-cli e il core 3.1.1, PlatformIO non serve più. Non rilanciarlo senza capire cosa era stato segnalato.

## Firmware
- [x] Unità antenna: prima versione scritta (PlatformIO, compila TX e RX), da provare sul banco.
- [ ] Taratura sul banco: corrente NEMA11, R_SENSE, soglia StallGuard, passi della versione RX.
- [x] Doppio driver del motore RX (STEP/DIR o CAN) scelto dal pannello: scritto, compila (protocollo v3).
- [ ] Prova del SERVO42D via CAN (SN65HVD230, GPIO 22/23; 120 Ω a bordo e velocità del bus da verificare) e via STEP/DIR.
- [ ] Protocollo CAN del SERVO42D: verificare sul manuale ufficiale CRC e comandi 0xFE, 0x91, 0xF1, 0x31, 0x8C; parametri di azzeramento nel menu del driver.
- [x] Collegamento ESP-NOW cifrato con heartbeat tra pannello e antenna, provato sulle schede il 21/11/2026 (canale 3 del router, chiavi di esempio).
- [ ] Chiavi ESP-NOW vere (`segreti.h`) su pannello e antenna: oggi entrambi usano quelle di esempio.
- [x] Associazione con il pulsante BOOT dell'antenna e conferma sul pannello: provata sulle schede il 21/11/2026.
- [x] Antenna: ricerca automatica del canale quando il pannello tace (ACK), provata il 21/11/2026.
- [ ] Display OLED dell'antenna: disturbo sui 40 e 20 m acceso e spento (con la radio).
- [x] Requisiti e mockup del pannello 7B approvati (docs/software/03-unita-controllo.md).
- [x] Firmware del pannello 7B, prima versione: driver, collegamento, barra, schede, Sintonia, temi, Impostazioni di base (compila).
- [x] Pannello sulla scheda vera (21/11/2026): schermo, touch, colori, luminosità, ricerca reti WiFi funzionano.
- [x] Pannello: collegamento ESP-NOW con l'antenna sulla scheda vera (21/11/2026).
- [x] Pannello: Calibrazione e Diagnostica (protocollo v2: flag, CAL_LEGGI, CAL_ELIMINA), compilano.
- [x] Pannello: Memorie, Propagazione, Decoder CW/RTTY, screensaver, Impostazioni complete, WiFi/NTP/indici (compilano; algoritmi provati sul PC).
- [ ] Decoder: prova con segnali veri dalla radio.
- [x] QRZ.com: ricerca nominativi e logbook scritti; HTTPS e risposta con chiave errata provati sulla scheda.
- [ ] QRZ.com: prova con l'account dell'utente (ricerca; logbook se abbonato) e verifica del formato ADIF di FETCH.
- [x] Pagina web del pannello: provata dal Mac (pagina, stato, schermo, tocco, loopant.local).
- [ ] Pagina web: codice di accesso facoltativo? (oggi nessuna password).
- [ ] Console sulla USB nativa: da provare collegando il cavo all'altra USB-C.
- [x] Pannello: font con lettere accentate, "·" e "—" (Montserrat Latin-1 con lv_font_conv).
- [x] Pannello: icone del mockup, disegnate con linee e archi LVGL.
- [x] Verso del PWM della retroilluminazione: 100 − luminosità è corretto (verificato sulla scheda).
- [ ] Ingresso audio del decoder: GPIO6 da verificare sullo schema; circuito d'ingresso da provare.
- [ ] Font monospazio per i temi a fosfori: servono circa 1,4 MB di flash (partizione app più grande) e un font per tema in tutta l'interfaccia.
- [ ] Mappa per la grey line in flash (Natural Earth), una per tema.
- [ ] Indici solari: verificare formato e condizioni d'uso del feed hamqsl.com; con il WiFi l'antenna va sul canale del router.
- [x] Ora esatta sul pannello: NTP via WiFi oppure impostazione dalla console.
- [ ] Verificare band plan IARU R1 e frequenze delle memorie predefinite.
- [ ] UART per il futuro CI-V sul 7B (schema elettrico).
- [ ] Procedura di calibrazione e tabella posizione ↔ frequenza.
