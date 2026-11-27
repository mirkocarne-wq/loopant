# Punti aperti

Da chiudere. Quando uno è risolto, spostare il risultato nel file di memoria giusto e toglierlo da qui.

## Misure
- [ ] Variabile 1000 pF: capacità minima e massima; continuità albero–telaio; uno o due statori isolati (decide dove vanno i capi dell'anello).
- [ ] Diametro dell'albero del variabile (giunto 8→6 mm?).
- [ ] Riduttore: foro di ingresso da 5 mm (albero SERVO42D) e albero di uscita (8 mm?).
- [ ] Tubi di rame/ottone e PTFE: disponibilità e misure reali con il calibro (diametro esterno e parete).
- [ ] Condensatore TX: confermare le tavole per il tornitore (`hardware/condensatore-tx/tavole-condensatore-tx.pdf`), spallamento del manicotto al posto degli anelli in nylon, tappo isolante in POM; manicotto in PTFE continuo (tubo 22/25 o 18/20, barra forata o foglio avvolto); misure delle rondelle in PTFE da 2 mm già prese (per fermi e distanziali); treccia piatta di rame stagnato per il pistone.
- [ ] Modulo NEMA11: corrente nominale, finecorsa inclusi?, giunti.
- [ ] Rumore dell'alimentatore da 24 V sui 40 e 20 m.
- [ ] Efficienza reale e Q dell'anello (NanoVNA).
- [ ] Anello RX: induttanza vera, capacità parassite e minimo vero del variabile (con il finecorsa 5° prima del
  fermo) per scegliere C1 e C2; adattamento della spira d'accoppiamento da 3,5 a 29,7 MHz (8:1 in frequenza).

## Progetto
- [ ] Disegno quotato della scatola RX e del gruppo TX.
- [ ] Relè dei condensatori in serie (RX, 17–10 m): scelta dei relè e di C1/C2 (compensatori?), posto nella scatola
  del variabile, scheda con transistor e diodi; poi prova con `serie` e calibrazione di 17–10 m.
- [ ] Contenitore stampato RX (`hardware/contenitore-rx/`): misurare squadra, riduttore, SERVO42D, giunto, millefori
  e OLED, correggere i parametri e stampare la piastra di prova.
- [ ] Assieme dell'antenna (`hardware/contenitore-rx/assieme.py`): confermare loop di accoppiamento (20 cm in cima, tubetto da 6 mm o RG-58, proposta), cima stampata (`cima.py`: sostegno, cappello, fondo con SO-239) con il raccordo stampato in due metà (T e braccio) sul palo in PVC da 40; regolare il loop piccolo per il ROS minimo.
- [ ] Gruppo palo in PVC Ø 40 (`hardware/contenitore-rx/gruppo-palo.pdf`): fissaggio al balcone (ringhiera, muro?) in due punti, rinforzo isolante dentro il palo (tondino di legno Ø 32 o vetroresina), diametro vero del palo (raccordo stampato con fessura di serraggio, foro Ø 40,4).
- [ ] Scatola del variabile: diametro del perno (6 mm?), fori M5 di sotto al centro della larghezza? tiranti più bassi delle linguette?, morsetti degli statori, interasse dei
  capi dell'anello, peso reale dell'anello; piatto da almeno 250 mm (o perno accorciato).

## Strumenti
- [x] Avviso malware al primo `pio run` (21/11/2026): causa non trovata nei log; l'antenna ora si compila con
  arduino-cli e il core 3.1.1, PlatformIO non serve più. Non rilanciarlo senza capire cosa era stato segnalato.

## Firmware
- [x] Unità antenna: prima versione scritta (PlatformIO, compila TX e RX), da provare sul banco.
- [ ] Taratura sul banco: corrente NEMA11, R_SENSE, soglia StallGuard, passi della versione RX.
- [x] Driver del motore che segue il tipo di antenna (TX STEP/DIR, RX CAN), firmware unico: provato sulle schede.
- [x] Transceiver CAN SN65HVD230 su GPIO 17/16 (scheda di espansione S/V/G): provato con `prove/prova_can` il 21/11/2026, 10/10.
- [x] SERVO42D via CAN con azzeramento sui suoi finecorsa: provato il 21/11/2026 (500 kbit/s, indirizzo 1; HmDir invertito nel menu).
- [ ] Resistenza da 120 Ω sulla scheda del transceiver: da verificare (il bus funziona).
- [ ] Finecorsa ottici sul SERVO42D: escursione vera del variabile (oggi 190°, `ESCURSIONE`) e verso della ruota (normale o specchiata). Alimentazione da EVCC (5 V), livello attivo alto e zero verso CCW: provati il 25/11/2026.
- [ ] Protocollo CAN del SERVO42D: confrontare con il manuale ufficiale V1.0.9 CRC e comandi 0xFE, 0x91, 0xF1, 0x8C (0x9E, 0x90, 0x34, 0x33, 0xF7 già presi dal manuale).
- [x] Finecorsa RX: zero su En e limite CW su Dir (rimappatura), EndLimit, HmTrig High; zero e limite CW provati il 25/11/2026.
- [ ] Finecorsa RX: jog libero della manopola fino a 0 sul finecorsa dello zero (non deve dare errore).
- [ ] SERVO42D: scritture via CAN rifiutate (stato 0) per qualche minuto il 25/11/2026, poi accettate: con il menu aperto?
- [ ] Corrente su EVCC del SERVO42D (massimo 20 mA) con i due finecorsa ottici.
- [x] Collegamento ESP-NOW cifrato con heartbeat tra pannello e antenna, provato sulle schede il 21/11/2026 (canale 3 del router, chiavi di esempio).
- [ ] Chiavi ESP-NOW vere (`segreti.h`) su pannello e antenna: oggi entrambi usano quelle di esempio.
- [x] Associazione con il pulsante BOOT dell'antenna e conferma sul pannello: provata sulle schede il 21/11/2026.
- [x] Antenna: ricerca automatica del canale quando il pannello tace (ACK), provata il 21/11/2026.
- [ ] Display OLED dell'antenna: disturbo sui 40 e 20 m acceso e spento (con la radio).
- [x] Requisiti e mockup del pannello 7B approvati (docs/software/03-unita-controllo.md).
- [x] Firmware del pannello 7B, prima versione: driver, collegamento, barra, schede, Sintonia, temi, Impostazioni di base (compila).
- [x] Pannello sulla scheda vera (21/11/2026): schermo, touch, colori, luminosità, ricerca reti WiFi funzionano.
- [x] Pannello da 5" (ESP32-S3-Touch-LCD-5B): schermo, tocco e retroilluminazione provati (23/11/2026).
- [x] Manopola associata al pannello 5B dal suo menu; menu e decoder acceso/spento provati (23/11/2026).
- [ ] Pannello 5B: prova completa con l'antenna (associati il 26/11/2026).
- [x] Copia su microSD: salvataggio e ripristino provati sul 5B il 26/11/2026 (6 punti riscritti e riletti).
- [ ] Copia su microSD: prova sul 7B; parametri dell'antenna (velocità, relè per banda, CAN) non ancora compresi.
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
- [x] Ingresso audio del decoder: GPIO6 sul connettore J8 «GPIO» del 7B (schema Waveshare).
- [x] Manopola remota (Waveshare ESP32-S3-Knob): hardware, associazione e decoder con il segnale di prova provati (21/11/2026).
- [x] Manopola: ritocco fine con l'antenna RX (JOG libero), provato il 21/11/2026.
- [ ] Manopola: TUNE dopo la calibrazione; microfono con la radio vera.
- [x] Antenna ricaricata con il firmware delle librerie comuni: collegamento con il pannello attivo (21/11/2026).
- [ ] Ingresso J8 del pannello (circuito con MCP6002): facoltativo, ora c'è la manopola.
- [ ] Font monospazio per i temi a fosfori: servono circa 1,4 MB di flash (partizione app più grande) e un font per tema in tutta l'interfaccia.
- [ ] Mappa per la grey line in flash (Natural Earth), una per tema.
- [ ] Indici solari: verificare formato e condizioni d'uso del feed hamqsl.com; con il WiFi l'antenna va sul canale del router.
- [x] Ora esatta sul pannello: NTP via WiFi oppure impostazione dalla console.
- [ ] Verificare band plan IARU R1 (80–10 m), limiti dei 60 m in Italia e frequenze delle memorie predefinite.
- [x] UART per il futuro CI-V sul 7B: connettore H3 «UART2» (GPIO 43/44) con SW1 su UART2 (scollega il CH343).
- [ ] Procedura di calibrazione e tabella posizione ↔ frequenza.
