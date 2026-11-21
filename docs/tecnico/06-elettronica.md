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
- Finecorsa: 2, con pull-up e filtro RC/100 nF.
- Versione RX: SERVO42D pilotato dallo stesso ESP32 con **STEP/DIR/EN** (modo a impulsi) oppure via CAN.

### Pinout suggerito (da confermare in fase di cablaggio)
| Funzione | GPIO | Note |
|---|---|---|
| STEP / DIR / EN | nell'intervallo 16–23 | Nel firmware: STEP 18, DIR 19, EN 21 |
| UART TMC2209 | 25 / 26 | Nel firmware: TX 25, RX 26 |
| Finecorsa | 27 / 14 | Nel firmware: MIN 27, MAX 14 |
| Ingressi analogici | solo ADC1 (32–39) | L'ADC2 non funziona con WiFi/ESP-NOW |
| CAN (solo SERVO42D via CAN) | TX 22 / RX 23 | Transceiver Waveshare SN65HVD230 (3,3 V) |
| Da evitare | 0, 2, 12, 15 | Pin di strapping |

I valori sono in `firmware/antenna/include/config.h`.

## Unità di controllo (casa)
- **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo GT911) — scelta (sostituisce il 5" previsto prima).
  Dettagli della scheda e requisiti: [../software/03-unita-controllo.md](../software/03-unita-controllo.md).
- Valutata anche la diymore 3,5" (320×480): più piccola, documentazione da verificare.
  Non adatta all'antenna: pochi GPIO e schermo inutile.

### Ingresso audio per il decoder CW/RTTY
- Il firmware usa l'ADC1 dell'ESP32-S3 su **GPIO6** (`cfg::PIN_AUDIO`): sembra l'unico ADC1 libero sul 7B, perché
  GPIO1-5 e 7-10 sono di schermo, touch e I2C. **Da verificare sullo schema** (connettore "Sensor"?).
- Collegamento proposto (da verificare):
  ```
  uscita linea radio ── trasformatore 1:1 ── 10 µF ──┬── GPIO6
                                                      ├── 10 kΩ ── 3,3 V
                                                      └── 10 kΩ ── GND
  ```
  Il partitore porta l'ingresso a metà tensione; il segnale deve restare sotto circa 1 V picco-picco.
  Il trasformatore evita anelli di massa con la radio.
- Alternativa: modulo ADC I2S. Dettagli: [../software/08-decoder.md](../software/08-decoder.md).

## CAN (solo se il SERVO42D si usa via CAN)
- Transceiver: **Waveshare SN65HVD230 CAN Board** (già in possesso), alimentato a 3,3 V come l'ESP32:
  nessun convertitore di livello.

  | SN65HVD230 | Collegamento |
  |---|---|
  | 3V3 | 3,3 V dell'ESP32 |
  | GND | GND comune (ESP32, SERVO42D, alimentazione) |
  | CTX | GPIO 22 (TX del controller CAN dell'ESP32, non incrociato) |
  | CRX | GPIO 23 (RX) |
  | CANH / CANL | CANH / CANL del SERVO42D, doppino intrecciato corto |

- La scheda Waveshare ha a bordo una resistenza da 120 Ω (da verificare sulla scheda). Con un solo slave e
  cavo corto basta quella: non aggiungere altri terminatori.
- Velocità del bus: deve essere uguale a quella impostata nel menu del SERVO42D (500 kbit/s di solito; da verificare).
- Il firmware per ora pilota il SERVO42D in STEP/DIR/EN; il controllo via CAN è da scrivere e provare.
- Il manuale non indica isolamento dell'interfaccia: considerarla **non isolata**.
  Con l'ESP32 nella scatola il cavo CAN è corto e non serve isolatore.
- Con un cavo lungo in futuro: transceiver isolato (ISO1050 con DC-DC, 3,3 V).

## Wattmetro / ROS (futuro, TX)
- Accoppiatore direzionale + due rivelatori AD8307 + ADC (ADS1115) letto dall'ESP32; soglie di allarme.
- Meglio vicino alla radio.
