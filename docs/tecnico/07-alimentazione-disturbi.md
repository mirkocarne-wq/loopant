# 07 — Alimentazione e disturbi HF

## Alimentazione
- **24 V, 2,5 A**. Alimentatore LED BRIMETI IP67 60 W valutato: la corrente va bene, ma è un commutatore economico
  e **può disturbare l'HF**.
- Metterlo in casa, lontano da radio e anello; ferriti (3–5 giri) in ingresso e uscita; eventuale filtro EMI.
- Possibilità di **spegnere il 24 V** quando il motore è fermo (MOSFET/relè comandato dall'ESP32).
- Convertitore 24→5 V per l'ESP32 con **ingresso fino a 36–40 V**, filtro LC e ferrite.
- Fusibile 2 A. Elettrolitico 100 µF 35 V vicino al TMC2209.
- Cavo schermato con ferriti a entrambi i capi.

## Rumore e disturbi
- Il cavo fa da antenna: **ferriti** (toroidi mix 31/43) a entrambi i capi, cavo schermato con lo schermo a massa da un solo lato.
- Coassiale e cavo dell'ESP32 incrociati a 90° e lontani dall'anello.
- Motore e driver spenti durante l'ascolto.
