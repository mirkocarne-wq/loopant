# 06 — Elettronica

## Schema a blocchi
```
[ESP32-S3 7B touch, in casa]  <-- ESP-NOW -->  [ESP32 DevKit, nella scatola all'antenna]
     tastierino, stato                          TMC2209 + motore, finecorsa, (AD8307)
```
Il software è descritto in [`../software/`](../software/README.md).

## Unità antenna
- **ESP32 DevKit a 38 pin** (normale, senza schermo), nella scatola, lontano da anello e motore.
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
| Da evitare | 0, 2, 12, 15 | Pin di strapping |

I valori sono in `firmware/antenna/include/config.h`.

## Unità di controllo (casa)
- **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo GT911) — scelta (sostituisce il 5" previsto prima).
  Dettagli della scheda e requisiti: [../software/03-unita-controllo.md](../software/03-unita-controllo.md).
- Valutata anche la diymore 3,5" (320×480): più piccola, documentazione da verificare.
  Non adatta all'antenna: pochi GPIO e schermo inutile.

### Ingresso audio per il decoder CW/RTTY (da definire)
- Il 7B non documenta un ingresso audio. Dall'uscita linea della radio servirà un modulo ADC I2S
  oppure un partitore verso un ingresso analogico libero, meglio con trasformatore audio 1:1 di isolamento.
  Dettagli: [../software/08-decoder.md](../software/08-decoder.md).

## CAN (solo se il SERVO42D si usa via CAN)
- Il manuale non indica isolamento dell'interfaccia: considerarla **non isolata**.
  Con l'ESP32 nella scatola il cavo CAN è corto e non serve isolatore.
- Per un solo slave, secondo il manuale, **non servono i terminatori da 120 Ω**.
- Con un cavo lungo in futuro: transceiver isolato (ISO1050 con DC-DC, 3,3 V).

## Wattmetro / ROS (futuro, TX)
- Accoppiatore direzionale + due rivelatori AD8307 + ADC (ADS1115) letto dall'ESP32; soglie di allarme.
- Meglio vicino alla radio.
