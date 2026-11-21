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
- [ ] macOS ha segnalato un malware durante il primo `pio run` (21/11/2026): file non identificato; il download si era
  fermato a un file vuoto. Capire cosa è stato segnalato prima di rilanciare PlatformIO.

## Firmware
- [x] Unità antenna: prima versione scritta (PlatformIO, compila TX e RX), da provare sul banco.
- [ ] Taratura sul banco: corrente NEMA11, R_SENSE, soglia StallGuard, passi della versione RX.
- [ ] Prova del SERVO42D via CAN e via STEP/DIR.
- [ ] Prova del collegamento ESP-NOW con heartbeat e cifratura tra le due schede.
- [x] Requisiti e mockup del pannello 7B approvati (docs/software/03-unita-controllo.md).
- [x] Firmware del pannello 7B, prima versione: driver, collegamento, barra, schede, Sintonia, temi, Impostazioni di base (compila).
- [x] Pannello sulla scheda vera (21/11/2026): schermo, touch, colori, luminosità, ricerca reti WiFi funzionano.
- [ ] Pannello: collegamento ESP-NOW con l'antenna sulla scheda vera.
- [x] Pannello: Calibrazione e Diagnostica (protocollo v2: flag, CAL_LEGGI, CAL_ELIMINA), compilano.
- [x] Pannello: Memorie, Propagazione, Decoder CW/RTTY, screensaver, Impostazioni complete, WiFi/NTP/indici (compilano; algoritmi provati sul PC).
- [ ] Decoder: prova con segnali veri dalla radio.
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
