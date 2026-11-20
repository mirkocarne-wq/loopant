# 03 — Versione RX: condensatore e meccanica

## Condensatore
- Variabile ad aria 1000 pF "1000V CC" (Jieotwice, Amazon).
- Da misurare con il NanoVNA: capacità minima e massima, **continuità albero–telaio** (probabile contatto strisciante).
- Per i 20 m servono circa 50 pF: se il minimo è più alto, quella banda non è raggiungibile.

## Motore e riduttore
- Motore: **NEMA17 + MKS SERVO42D CAN** (closed loop, encoder 16384 conteggi, OLED) — già ordinato.
- Riduttore: **STEPPERONLINE planetario 20:1, gioco 30 arc-min**, per NEMA17 — acquistato.
- Con 20:1 il passo all'uscita è di circa 0,001° per conteggio (circa 0,006 pF): la risoluzione non è un limite.
- **Gioco 30 arc-min (0,5° ≈ 2,7 pF)**: conta sui 20 m (circa centinaia di kHz). L'encoder è sul motore e non lo vede.
  **Regola: arrivare sempre dallo stesso verso.** Il gioco diventa un offset costante e la tabella lo assorbe.
- Verificare che il foro di ingresso del riduttore sia da 5 mm (come l'albero del SERVO42D)
  e il diametro dell'albero di uscita (spesso 8 mm).

## Albero e giunti
- Albero e telaio del variabile portano RF: serve **isolamento**.
- **Barra in ABS da 8 mm** (acquistata), lunga 8–10 cm tra i due giunti metallici.
- Catena: albero riduttore (8 mm) → giunto → asta ABS → giunto 8→6 mm → albero del variabile
  (misurare l'albero con il calibro).
- Fissaggio sull'ABS: i grani schiacciano la plastica; meglio una spianatura (D-cut), uno spinotto trasversale o epossidica.
- Verificare con il tester che non ci sia continuità tra riduttore/motore e telaio/albero del variabile.
- L'ABS soffre i raggi UV: vernice o protezione.

## Scatola stagna in plastica
- IP65 o superiore, circa 25×15×10 cm (variabile, riduttore, motore, SERVO42D).
- Variabile su distanziali in nylon, lontano da pareti e motore. Piattine di rame corte e larghe verso gli statori.
- Passacavi PG stagni per alimentazione, segnali e coassiale (ferriti all'esterno).
  I due capi dell'anello entrano da fori con gommino/manicotto in silicone; tappi sulle estremità dei tubi.
- **Condensa**: sacchetti di silica gel e una **valvola di sfiato a membrana** (tipo Gore).
- Motore libero (driver disabilitato) dopo ogni spostamento.
