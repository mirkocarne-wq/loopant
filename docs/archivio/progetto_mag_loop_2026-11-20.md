# Progetto Mag Loop 40 m / 20 m — RX e TX 100 W

Documento riassuntivo della progettazione (aggiornato al 20/11/2026).
Le stime di calcolo sono mie e vanno verificate con misure reali (NanoVNA). Dove un dato non è verificato è indicato.

---

## 1. Obiettivo

- Antenna magnetic loop per **40 m e 20 m**, da usare su balcone (circa 4 m).
- **Due versioni**:
  1. **RX** (solo ricezione): anello 1 m, variabile a lamelle da 1000 pF già in possesso. Ricevitore: Malahit DSP2 (e in futuro Icom IC-7300 MK2).
  2. **TX 100 W**: anello 1,5 m, condensatore a "stantuffo" (tubo in tubo con PTFE), motorizzato, con wattmetro/ROS.
- Sintonia **automatica**: sull'ESP32 con touch in casa si digita la frequenza e l'antenna si porta in risonanza.

## 2. Principi e numeri di riferimento

- Risonanza: f = 1 / (2π√(LC)).
- Induttanza anello circolare: L ≈ μ0·(D/2)·(ln(8·(D/2)/a) − 2), con a = raggio del tubo.
- Tensione sul condensatore: Vc = √(P·Q·Xc). Corrente: I = √(P·Q/Xc).
- Banda molto stretta: circa 40–50 kHz sui 20 m, circa 28 kHz sui 40 m → va riaccordata a ogni cambio di frequenza.
- Pattern a "otto", perpendicolare al piano dell'anello; nulli nel piano.

### Induttanze stimate (anello 1 m, circolare)

| Tubo | Diametro esterno | L stimata |
|---|---|---|
| 3/8" | 9,5 mm | circa 3,0 µH |
| 1/2" | 12,7 mm | circa 2,8 µH |
| 5/8" | 15,9 mm | circa 2,65 µH |
| 18 mm | 18 mm | circa 2,6 µH |
| 3/4" | 19 mm | circa 2,55 µH |

### Capacità richieste (stime, con circa 4 pF parassiti)

- Anello **RX 1 m** (circa 2,6–2,8 µH): 40 m circa 180–195 pF, 20 m circa 45–50 pF.
- Anello **TX 1,5 m, tubo 22 mm** (L circa 4 µH): 40 m circa 118–130 pF, 20 m circa 28–33 pF.
- Per il TX serve una escursione di circa **100 pF**.

### Stress del condensatore in TX 100 W (Q ≈ 300, stima)

- Circa 3,3 kV di picco sui 40 m, circa 4,7 kV di picco sui 20 m.
- Corrente circa 9–13 A.
- Il variabile a lamelle da 1000 pF (spaziatura circa 1 mm, "1000 V CC" dichiarati) **non è adatto al TX**; solo prove a 1–2 W.

### Rendimento stimato (mio calcolo, da verificare)

- Anello DIY 1,5 m: circa 30% sui 40 m, circa 85% sui 20 m.
- Riferimenti commerciali: Käferlein AMA-41 (0,8 m, 100 W): 21% a 7 MHz, 76% a 14 MHz.

---

## 3. Anello

### Versione RX

- **Diametro 1 m** (raggio 50 cm sull'asse del tubo), lunghezza di tubo circa **3,3 m** (3,14 m di circonferenza + margine per i capi e i tagli).
- **Tubo di rame**: scelta fatta tra **1/2" (12,7 mm)** e 5/8" (15,9 mm) di rame ricotto da frigorista. Il 5/8" perde circa 0,5 dB in più rispetto ai 18 mm, l'1/2" circa 1,5 dB, il 3/8" circa 2,8 dB. In ricezione la differenza è trascurabile.
- Rotolo da 5 m. Curvatura con sagoma circolare (compensato, tavolo rotondo, hula hoop da 1 m), molla piegatubi o sabbia. Piega su raggio circa 48 cm per compensare il ritorno elastico.
- Giunzioni a stagno o raccordi a compressione ben serrati. Le resistenze di contatto contano più del diametro del tubo.
- Anello di accoppiamento: circa 1/5 del diametro (circa 20 cm), cavo coassiale RG58/RG213 (circa 70 cm), con ferriti sul coassiale.

### Versione TX

- Anello 1,5 m, tubo **22 mm**. Il rame è la scelta migliore (corrente 9–13 A).

### Posizionamento sul balcone

- Centro dell'anello ad almeno 1,2–1,5 m dal pavimento, ad almeno mezzo diametro da pareti e ringhiere metalliche. Palo in vetroresina/PVC.
- Si può ruotare l'anello per annullare un disturbo (nullo nel piano).
- In TX a 100 W: persone e animali a più di 2 m.

---

## 4. Versione RX — condensatore a lamelle e meccanica

### Condensatore

- Variabile ad aria 1000 pF "1000V CC" (Jieotwice, Amazon). Da misurare con il NanoVNA: capacità minima e massima, **continuità albero–telaio** (probabile contatto strisciante). Per i 20 m servono circa 50 pF: se il minimo è più alto, quella banda non è raggiungibile.

### Riduttore e motore

- Motore: **NEMA17 + MKS SERVO42D CAN** (closed loop, encoder 16384 conteggi, OLED) — già ordinato.
- Riduttore: **STEPPERONLINE planetario 20:1, gioco 30 arc-min**, per NEMA17 — acquistato.
- Con 20:1 il passo all'uscita è di circa 0,001° per conteggio (circa 0,006 pF), quindi la risoluzione non è un limite.
- **Gioco 30 arc-min (0,5° ≈ 2,7 pF)**: è importante sui 20 m (circa centinaia di kHz). L'encoder è sul motore e non lo vede. **Regola del firmware: arrivare sempre da un solo verso.** Il gioco diventa un offset costante e la tabella lo assorbe.
- Verificare che il foro di ingresso del riduttore sia da 5 mm (come l'albero del SERVO42D) e il diametro dell'albero di uscita (spesso 8 mm).

### Albero e giunti

- Albero e telaio del variabile portano RF: serve **isolamento**.
- **Barra in ABS da 8 mm** (acquistata), lunga 8–10 cm tra i due giunti metallici.
- Schema: albero riduttore (8 mm) → giunto → asta ABS → giunto 8→6 mm → albero del variabile (misurare l'albero con il calibro).
- Fissaggio sull'ABS: i grani schiacciano la plastica; meglio una spianatura (D-cut), uno spinotto trasversale o epossidica.
- Verificare con il tester che non ci sia continuità tra riduttore/motore e telaio/albero del variabile.
- L'ABS soffre i raggi UV: vernice o protezione.

### Scatola stagna in plastica

- IP65 o superiore, circa 25×15×10 cm (variabile, riduttore, motore, SERVO42D).
- Variabile montato su distanziali in nylon, lontano da pareti e motore. Piattine di rame corte e larghe verso gli statori.
- Passacavi PG stagni per alimentazione, segnali e coassiale (ferriti all'esterno). I due capi dell'anello entrano da fori con gommino/manicotto in silicone; tappi sulle estremità dei tubi.
- **Condensa**: sacchetti di silica gel e una **valvola di sfiato a membrana** (tipo Gore).
- Scatola in basso, vicino al palo; anello di accoppiamento in alto.
- Motore libero (driver disabilitato) dopo ogni spostamento.

---

## 5. Versione TX — condensatore a stantuffo (tubo in tubo, PTFE)

### Principio

- Un tubo di rame interno scorre in un tubo esterno, con un **manicotto in PTFE** in mezzo. L'accoppiamento è capacitivo: **nessun contatto strisciante**.
- Capacità circa 7 pF/cm (PTFE 2 mm) o circa 9 pF/cm (PTFE 1,5 mm). Perdita nel dielettrico circa 6 W a 100 W / 40 m (stima).
- Il rapporto 1/f² è lineare con la posizione, quindi bastano due parametri per banda per la calibrazione.

### Dimensioni (progetto base)

| Parte | Misura |
|---|---|
| Tubo esterno | rame 28×1 (interno 26 mm), circa 20–22 cm |
| Pistone (tubo interno) | rame 22×1, circa 22 cm |
| Manicotto PTFE | interno 22 mm, esterno 25 mm (parete 1,5 mm) |
| Asta isolante | 8 mm, vetroresina o nylon, circa 16 cm |
| **Corsa** | **150 mm** (servono circa 11–14 cm per 20 m–40 m) |

### Tappi e dettagli

- **Pistone**: tappo = disco di rame di circa 20 mm saldato **dentro** l'estremità che entra nel tubo esterno (un tappo a saldare commerciale va sopra il tubo e allargherebbe il diametro). Cordone lisciato, spigolo arrotondato (campo elevato).
- **Tubo esterno**: tappo normale a saldare sull'estremità di fondo, con **foro di sfiato da 2 mm** rivolto verso il basso (evita l'effetto pistone e fa uscire umidità/acqua).
- **Distanza minima**: a pistone tutto dentro, almeno **10 mm** tra il suo tappo e il fondo del tubo esterno, con un fermo meccanico (4–5 kV di picco).
- Il PTFE deve **sporgere** dal tubo esterno di almeno 1–2 cm (scarica sul bordo).
- Pulizia: lima, carta fine e alcol; nessuna sbavatura.
- Il PTFE non si incolla: bloccarlo nel tubo esterno con anelli o spine in nylon.

### Alternativa con tubi più facili da trovare

- **Pistone 18×1 e esterno 22×1** (rame da idraulica comune): interno del 22×1 = 20 mm, quindi PTFE interno 18 / esterno 20 mm (parete 1 mm).
- Stima circa **11 pF/cm**, corsa necessaria circa 9–10 cm.

### Materiali

- Rame consigliato (resistenza più bassa). Ottone liscio e nudo accettabile (perdita stimata circa 0,7 W per tubo contro circa 0,35 W); mix rame/ottone senza problemi di corrosione.
- **Tubi per tende**: spesso acciaio o alluminio con finitura "ottone" → controllare con una calamita e misurare con il calibro diametro e parete.
- Il tubo prolunga doccia Leroy Merlin valutato (ottone, circa 24 mm, filettatura 3/4", 22,46 €) è stato scartato: [scheda](https://www.leroymerlin.it/prodotti/valaz-tubo-prolunga-doccia-oro-opaco-30-cm-con-protezione-pvd-96115471.html).

---

## 6. Movimento del condensatore TX

### Scelta fatta: modulo lineare con NEMA11

- Mini attuatore lineare a guida con **vite T6×1** e motore **NEMA11**, **corsa utile 150 mm** (tipo kit CNC), già allineato.
- Il **SERVO42D è stato abbandonato per il TX** (resta per la versione RX; ha anche ingresso a impulsi STEP/DIR/EN).
- Ciclo **aperto**: nessun encoder. Mitigazioni:
  1. Carico minimo (attrito del PTFE), corrente adeguata, rampe di accelerazione.
  2. **Zero a ogni cambio banda** con i finecorsa.
  3. **Ricerca del picco** sul segnale (S-meter in RX; ROS in TX).
  4. Rilevamento di blocco con StallGuard del TMC2209.
  5. Opzionale: sensore lineare (KTC-150) o encoder magnetico AS5600.
- Tempo di corsa completa: 150 giri; a 300 giri/min circa 30 s, a 600 giri/min circa 15 s.

### Alternativa valutata (non scelta)

- NEMA17 + vite **TR8×2** (300 mm) + guida **MGN9H 300 mm** con un pattino (fori pattino circa 15×16 mm, M3, da verificare sul disegno del venditore) + dado flottante, cuscinetto 608, giunto 5→8 mm. Il riduttore 20:1 qui non serve.

---

## 7. Elettronica e controllo

### Architettura

```
[ESP32-S3 5" touch, in casa]  <-- ESP-NOW -->  [ESP32 DevKit, nella scatola all'antenna]
     tastierino, stato                          TMC2209 + motore, finecorsa, (AD8307)
```

### Antenna

- **ESP32 DevKit a 38 pin** (normale, senza schermo), nella scatola, lontano da anello e motore.
- Driver **TMC2209** (STEP/DIR/EN), in **stealthChop**; driver disabilitato dopo ogni movimento.
- Corrente del NEMA11: da regolare sul valore nominale (0,5–1 A, controllare la scheda).
- Finecorsa: 2, con pull-up e filtro RC/100 nF.
- Pin suggeriti: STEP/DIR/EN su 16–23; UART TMC2209 su 25/26; finecorsa 27/14; ingressi analogici solo su **ADC1 (32–39)** (l'ADC2 non funziona con WiFi/ESP-NOW). Evitare i pin di strapping (0, 2, 12, 15).
- Per la versione RX: SERVO42D pilotato dallo stesso ESP32 con **STEP/DIR/EN** (modo a impulsi) oppure via CAN.

### Casa (controllo)

- **Waveshare ESP32-S3 Touch LCD 5"** (1024×600, touch capacitivo) — scelta. Usare gli esempi Waveshare con LVGL; buffer piccoli se WiFi e grafica insieme.
- Schermata prevista: frequenza grande, tastierino, pulsanti 40 m / 20 m, stato (fermo / in movimento / errore), posizione, "Tune", "Stop", "Zero".
- Valutata anche la scheda diymore 3,5" (320×480): più piccola, documentazione da verificare ([manuale](https://co.manuals.plus/ae/1005008555464310)). Non adatta all'antenna: pochi GPIO e schermo inutile.

### Collegamento ESP-NOW

- MAC fissi nel firmware (salvati in NVS), **stesso canale WiFi** (non collegare a un router), **cifratura** con chiavi PMK/LMK.
- Messaggi: `GOTO freq`, `STOP`, `ZERO`, `STATO`; risposta con posizione, stato e numero di sequenza.
- **Heartbeat** ogni secondo; timeout → motore fermo e driver libero.
- Pairing opzionale con pulsante (scambio MAC in broadcast).

### Alimentazione

- **24 V, 2,5 A** (alimentatore LED BRIMETI IP67 60 W valutato: la corrente va bene, ma è un commutatore economico e **può disturbare l'HF**).
- Metterlo in casa, lontano da radio e anello; ferriti (3–5 giri) in ingresso e uscita; eventuale filtro EMI.
- Possibilità di **spegnere il 24 V** quando non si muove il motore (MOSFET/relè comandato dall'ESP32).
- Convertitore 24→5 V per l'ESP32 con **ingresso fino a 36–40 V**, con filtro LC e ferrite. Fusibile 2 A. Elettrolitico 100 µF 35 V vicino al TMC2209.
- Cavo schermato con ferriti a entrambi i capi.

### Note sul CAN (se si usa il SERVO42D via CAN)

- Il manuale non indica isolamento dell'interfaccia: considerarla **non isolata**. Con ESP32 nella scatola il cavo CAN è corto e non serve isolatore.
- Per un solo slave, secondo il manuale, **non servono i terminatori da 120 Ω**.
- Se in futuro il cavo fosse lungo: transceiver isolato (ISO1050 con DC-DC, 3,3 V). Fonte: [manuale SERVO42D/57D](https://manualmachine.com/mks/servo42d/28566353-user-manual).

---

## 8. Tuning e calibrazione

1. **Tabella** posizione ↔ frequenza costruita con il **NanoVNA** (dip S11 sull'anello di accoppiamento); Q ≈ f0 / BW.
2. Arrivo **sempre da un solo verso** (assorbe il gioco).
3. **Zero** ai finecorsa a ogni cambio banda.
4. **Ricerca fine del picco** attorno alla posizione di tabella:
   - RX: segnale audio/S-meter (AGC manuale, senza attenuatore).
   - TX: ROS minimo a bassa potenza (circa 5 W), poi blocco e piena potenza.
5. Posizione salvata nella flash dell'ESP32.
6. In TX **non muovere il condensatore con RF applicata** ad alta potenza.

## 9. Sviluppi futuri

- **Interfaccia con Icom (CI-V) o Yaesu (CAT)**: lasciare libero un UART sulla scheda di controllo; funzione "segui la radio" che legge la frequenza e porta l'antenna in risonanza.
- **Wattmetro/ROS** per il TX: accoppiatore direzionale + due rivelatori AD8307 + ADC (ADS1115) letto dall'ESP32; protezione con soglie di allarme. Meglio vicino alla radio.
- Eventuale sensore lineare (KTC-150) o encoder AS5600 se si perdono passi.

## 10. Rumore e disturbi (HF)

- Cavo come antenna: **ferriti** (torroidi 31/43) a entrambi i capi, cavo schermato con schermo a massa da un solo lato.
- Coassiale e cavo dell'ESP32 incrociati a 90° e lontani dall'anello.
- Motore e driver spenti durante l'ascolto.

---

## 11. Lista acquisti (stato)

**Già acquistato / in possesso**
- Variabile a lamelle 1000 pF
- NEMA17 + MKS SERVO42D CAN
- Riduttore planetario 20:1 (STEPPERONLINE)
- Barra ABS 8 mm
- NanoVNA
- Modulo attuatore lineare NEMA11, T6×1, corsa 150 mm (scelto)

**Da acquistare**
- Rame 1/2" (o 5/8") in rotolo da 5 m (anello RX)
- Rame 22×1 e 28×1 (oppure 18×1 e 22×1), disco/tappi, PTFE su misura
- Asta isolante, giunti (8→6 mm, 5→8 mm)
- ESP32 DevKit 38 pin, **TMC2209**, microswitch finecorsa
- Convertitore 24→5 V (ingresso fino a 36–40 V), fusibile, elettrolitico, ferriti
- Scatole IP65 (RX e motore TX), passacavi, silica gel, valvola di sfiato
- Waveshare ESP32-S3 5" touch
- Cavo schermato, coassiale RG58/RG213 per accoppiamento
- Per il TX: wattmetro (AD8307 ×2, ADS1115, accoppiatore)

---

## 12. Da verificare / punti aperti

- Misure del variabile: capacità minima e massima, continuità albero–telaio.
- Diametro dell'albero del variabile (giunto 8→6 mm?) e fori del riduttore (5 mm in ingresso, 8 mm in uscita).
- Disponibilità dei tubi di rame/ottone e del PTFE; misura reale con il calibro (diametro esterno e parete).
- Rumore dell'alimentatore da 24 V sulle bande 40/20 m.
- Dettagli del modulo NEMA11 (corrente nominale, finecorsa inclusi?, giunti).
- Firmware: da scrivere (prova CAN o STEP/DIR, ESP-NOW, interfaccia touch, calibrazione).
- Disegno quotato della scatola RX e del gruppo TX.
- Efficienza reale e Q: da misurare.

---

## 13. Fonti consultate

- [Manuale MKS SERVO42D/57D CAN V1.0.3](https://manualmachine.com/mks/servo42d/28566353-user-manual)
- [TI ISO1050](https://www.ti.com/product/iso1050)
- [Copperhill, Isolated CAN Bus Breakout Board 3.3 VDC](https://copperhilltech.com/isolated-can-bus-breakout-board-3-3-vdc)
- [Manuale diymore ESP32-S3 3,5"](https://co.manuals.plus/ae/1005008555464310)
- [Leroy Merlin, tubo prolunga doccia (scartato)](https://www.leroymerlin.it/prodotti/valaz-tubo-prolunga-doccia-oro-opaco-30-cm-con-protezione-pvd-96115471.html)
