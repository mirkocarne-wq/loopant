# 06 — Elettronica

## Schema a blocchi
```
[ESP32-S3 7B touch, in casa]  <-- ESP-NOW -->  [ESP32 DevKit, nella scatola all'antenna]
     tastierino, stato                          TMC2209 + motore, finecorsa, (AD8307)
```
Il software è descritto in [`../software/`](../software/README.md).

## Unità antenna
- **ESP32 DevKit a 30 pin** (modulo WROOM-32 schermato, senza schermo LCD), nella scatola, lontano da anello e motore.
  Tutti i pin usati dal firmware ci sono sia sulla versione a 30 sia su quella a 38 pin.
- Driver **TMC2209** (STEP/DIR/EN), in **stealthChop**; driver disabilitato dopo ogni movimento.
- Corrente del NEMA11: da regolare sul valore nominale (0,5–1 A, controllare la scheda tecnica).
- Finecorsa: **antenna TX** sull'ESP32 (GPIO 27 MIN, 14 MAX, pull-up e filtro RC/100 nF), lo zero lo fa il firmware;
  **antenna RX** sul SERVO42D, che fa lo zero da solo (decisione del 21/11/2026): zero su En, limite CW su Dir,
  COM a EVCC, moduli alimentati da EVCC/EGND (rimappatura 9E; collegamenti in `../software/02-unita-antenna.md`).
- Versione RX: SERVO42D pilotato dallo stesso ESP32 **via CAN**. La versione TX accorda in STEP/DIR con il TMC2209.

### Pinout suggerito (da confermare in fase di cablaggio)
| Funzione | GPIO | Note |
|---|---|---|
| STEP / DIR / EN | nell'intervallo 16–23 | Nel firmware: STEP 18, DIR 19, EN 23 (libero all'accensione) |
| UART TMC2209 | 25 / 26 | Nel firmware: TX 25, RX 26 |
| Finecorsa (solo antenna TX) | 27 / 14 | Nel firmware: MIN 27, MAX 14; sull'antenna RX sono sul SERVO42D |
| Ingressi analogici | solo ADC1 (32–39) | L'ADC2 non funziona con WiFi/ESP-NOW |
| CAN (solo SERVO42D via CAN) | TX 17 (TX2) / RX 16 (RX2) | Transceiver Waveshare SN65HVD230 (3,3 V) |
| Display OLED 0,96" (I2C) | SDA 21 / SCL 22 | SSD1306 128×64 a 0x3C, pin I2C standard; bus libero per altri moduli I2C (es. ADS1115) |
| Relè C1 / C2 (solo antenna RX, 17–10 m) | 4 / 13 | Proposta del 26/11/2026: transistor NPN (o MOSFET logico) e diodo di ricircolo per relè; sul 13 resistenza da 10 kΩ verso massa (il 4 ha già la pull-down all'avvio) |
| Da evitare | 0, 2, 12, 15 | Pin di strapping |

I valori sono in `firmware/antenna/include/config.h`. Il 21/11/2026 EN e CAN sono stati spostati (prima EN 21,
CAN 22/23) per lasciare al display OLED i pin I2C standard. Il GPIO 4 non va bene per EN: all'accensione ha una
pull-down interna e il driver (EN attivo basso) resterebbe abilitato durante l'avvio.

### Relè dei condensatori in serie (solo antenna RX, proposta da costruire)
- Due relè a 5 V nella scatola del variabile ([03](03-rx-condensatore-meccanica.md)): K1 inserisce C1, K2 inserisce C2.
- Pilotaggio: GPIO 4 (K1) e 13 (K2) → resistenza da 1 kΩ → base di un NPN (es. BC337), bobina tra +5 V e
  collettore, diodo 1N4148 in parallelo alla bobina; sul 13 una resistenza da 10 kΩ tra base e massa tiene il relè
  spento durante l'avvio. A riposo (ESP32 spento o in avvio) il variabile resta da solo.
- Corrente delle bobine dal convertitore 24→5 V (circa 30–70 mA per relè, da verificare sul modello scelto).
- Sui fili verso la scatola del variabile: ferrite e 100 nF verso massa alle due estremità.

### Display OLED (facoltativo)
- Modulo ELEGOO 0,96" blu e giallo: SSD1306 128×64 su I2C, indirizzo 0x3C; le prime 16 righe sono gialle.
  Collegamenti: VCC a 3,3 V, GND, SDA al GPIO 21, SCL al GPIO 22 (provato sulla scheda il 21/11/2026).
- Il firmware funziona anche senza. Dopo 2 minuti senza cambi di stato si spegne, convertitore interno compreso:
  il suo eventuale disturbo sui 40 e 20 m è **da verificare** con la radio.

## Unità di controllo (casa)
- **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo GT911) — scelta (sostituisce il 5" previsto prima).
  Dettagli della scheda e requisiti: [../software/03-unita-controllo.md](../software/03-unita-controllo.md).
- Valutata anche la diymore 3,5" (320×480): più piccola, documentazione da verificare.
  Non adatta all'antenna: pochi GPIO e schermo inutile.

### Ingresso audio per il decoder CW/RTTY
- Il firmware usa l'ADC1 dell'ESP32-S3 su **GPIO6** (`cfg::PIN_AUDIO`), l'unico ADC1 libero sul 7B. Verificato
  sullo schema Waveshare il 21/11/2026: arriva al connettore **J8 «GPIO»** (3V3, GND, GP6).
- Altri connettori del 7B utili: **H1 «I2C»** (3V3, GND, SDA 8, SCL 9: stesso bus di touch e IO expander) e
  **H3 «UART2»** (seriale 0 dell'ESP32, GPIO 43/44): si attiva con il deviatore SW1 su UART2, che scollega il CH343
  («USB TO UART»); resta la USB nativa.
- **Circuito d'ingresso** (scelta del 21/11/2026; valori da verificare sul banco), alimentato dal 3,3 V di J8:
  ```
  radio (linea/cuffia) ─ T1 1:1 600 Ω ─ RV1 10 kΩ (livello) ─ C1 1 µF ─┬─ R1 10 kΩ ─┬─ R2 10 kΩ ─┬─ U1 (+)
                                                                       │            │            │
                                                              R3 100 kΩ a VREF   C2 6,8 nF    C3 3,3 nF
                                                                            (verso l'uscita) (a VREF)
  U1 (−) collegato all'uscita (inseguitore)
  U1 uscita ─ R4 1 kΩ ─┬─ GPIO6 (J8)
                        └─ C4 10 nF ─ GND
  VREF = 1,65 V: partitore 10 kΩ + 10 kΩ tra 3,3 V e GND, con 10 µF verso GND
  U1 = MCP6002 (operazionale rail-to-rail a 3,3 V), 100 nF tra 3,3 V e GND vicino all'integrato
  ```
  - T1 separa le masse di radio e pannello (niente ronzii né anelli di massa).
  - C1 e R3 tolgono la continua e polarizzano a metà alimentazione (l'ADC non legge tensioni negative).
  - R1, R2, C2, C3 con U1 formano un passa-basso Sallen-Key del secondo ordine a circa 3,4 kHz (Q circa 0,7):
    primo filtro anti-aliasing; il secondo è il FIR del firmware.
  - R4 e C4 isolano l'uscita dell'operazionale dal condensatore di campionamento dell'ADC.
  - Livello: con RV1 si regola per circa 1 V picco-picco sui segnali forti (fondo scala circa 2,8 V p-p);
    la barra del livello nella pagina Decoder aiuta.
- **Firmware**: ADC a 32 kHz, componente continua tolta, FIR passa-basso a 63 prese (finestra di Hamming, −6 dB a
  3,6 kHz) e riduzione a 8 kHz (`decoder/decimatore.*`). Sul PC: toni di CW e RTTY a 0,0 dB, frequenze che si
  ripiegherebbero sulla banda attenuate di circa 65 dB.
- Se servirà di più (spettro o waterfall fluidi), un secondo ESP32-S3 con ADC I2S PCM1808 a 24 bit che decodifica e
  manda il testo al pannello (I2C su H1, UART su H3 o ESP-NOW). Un ADC su I2C non conviene: quelli ad alta
  risoluzione sono troppo lenti per l'RTTY e quelli veloci sono a 12 bit come l'ADC interno.

## CAN (solo se il SERVO42D si usa via CAN)
- Transceiver: **Waveshare SN65HVD230 CAN Board** (già in possesso), alimentato a 3,3 V come l'ESP32:
  nessun convertitore di livello.

  | SN65HVD230 | Collegamento |
  |---|---|
  | 3V3 | 3,3 V dell'ESP32 |
  | GND | GND comune (ESP32, SERVO42D, alimentazione) |
  | CTX | GPIO 17, «TX2» sul DevKit (TX del controller CAN dell'ESP32, non incrociato) |
  | CRX | GPIO 16, «RX2» sul DevKit (RX) |
  | CANH / CANL | CANH / CANL del SERVO42D, doppino intrecciato corto |

- La scheda Waveshare ha a bordo una resistenza da 120 Ω (da verificare sulla scheda). Con un solo slave e
  cavo corto basta quella: non aggiungere altri terminatori.
- Velocità del bus: deve essere uguale a quella impostata nel menu del SERVO42D (500 kbit/s di solito; da verificare).
- Sull'antenna RX il motore è comandato via CAN (il tipo di antenna si sceglie dal pannello). I finecorsa sono
  sul SERVO42D (zero su En, limite CW su Dir, COM a EVCC) e l'azzeramento lo fa il driver: provato il 25/11/2026.
  Sensori: due finecorsa ottici a forcella (VCC 2,7–5 V; SIG basso a luce libera, alto con la bandierina nella
  forcella, quindi HmTrig High), alimentati da EVCC/EGND (5 V); meccanica in `hardware/contenitore-rx/`.
  EVCC regge 20 mA (manuale): i due sensori più un ingresso attivo dovrebbero starci, **da misurare**.
- Il manuale non indica isolamento dell'interfaccia: considerarla **non isolata**.
  Con l'ESP32 nella scatola il cavo CAN è corto e non serve isolatore.
- Con un cavo lungo in futuro: transceiver isolato (ISO1050 con DC-DC, 3,3 V).

## Wattmetro / ROS (futuro, TX)
- Accoppiatore direzionale + due rivelatori AD8307 + ADC (ADS1115) letto dall'ESP32; soglie di allarme.
- Meglio vicino alla radio.
