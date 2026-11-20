# 05 — Versione TX: movimento del condensatore

## Scelta: modulo lineare con NEMA11
- Mini attuatore lineare a guida con **vite T6×1** e motore **NEMA11**, **corsa utile 150 mm** (tipo kit CNC), già allineato.
- Il **SERVO42D non si usa per il TX** (resta sulla versione RX).
- Tempo di corsa completa: 150 giri; a 300 giri/min circa 30 s, a 600 giri/min circa 15 s.

## Ciclo aperto (senza encoder): mitigazioni
1. Carico minimo (attrito del PTFE), corrente adeguata, rampe di accelerazione.
2. **Zero a ogni cambio banda** con i finecorsa.
3. **Ricerca del picco** sul segnale (S-meter in RX; ROS in TX).
4. Rilevamento di blocco con StallGuard del TMC2209.
5. Opzionale: sensore lineare (KTC-150) o encoder magnetico AS5600.

## Alternativa valutata (non scelta)
- NEMA17 + vite **TR8×2** (300 mm) + guida **MGN9H 300 mm** con un pattino
  (fori pattino circa 15×16 mm, M3, da verificare sul disegno del venditore) + dado flottante, cuscinetto 608, giunto 5→8 mm.
  Il riduttore 20:1 qui non serve.
