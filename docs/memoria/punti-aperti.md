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

## Firmware
- [x] Unità antenna: prima versione scritta (PlatformIO, compila TX e RX), da provare sul banco.
- [ ] Taratura sul banco: corrente NEMA11, R_SENSE, soglia StallGuard, passi della versione RX.
- [ ] Prova del SERVO42D via CAN e via STEP/DIR.
- [ ] Prova del collegamento ESP-NOW con heartbeat e cifratura tra le due schede.
- [x] Requisiti e mockup del pannello 7B approvati (docs/software/03-unita-controllo.md).
- [ ] Firmware del pannello 7B: driver schermo/touch, schermate LVGL, temi.
- [ ] Ingresso audio per il decoder CW/RTTY sul 7B (modulo I2S o ingresso analogico, schema elettrico).
- [ ] Font per LVGL: cifre a 7 segmenti (es. DSEG7) e monospazio per i temi a fosfori, con lv_font_conv.
- [ ] Mappa per la grey line in flash (Natural Earth), una per tema.
- [ ] Indici solari via WiFi: fonte (feed hamqsl.com?), formato, condizioni d'uso; canale ESP-NOW = canale del router.
- [ ] Ora esatta sul pannello: NTP via WiFi o impostazione manuale (nessun RTC documentato).
- [ ] Verificare band plan IARU R1 e frequenze delle memorie predefinite.
- [ ] UART per il futuro CI-V sul 7B (schema elettrico).
- [ ] Toolchain del pannello: serve il core Arduino 3.x (Arduino IDE 2, pioarduino o ESP-IDF).
- [ ] Procedura di calibrazione e tabella posizione ↔ frequenza.
