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

## Contenitore stampato del motore e dell'elettronica (proposta da confermare)
- Modello parametrico in [`hardware/contenitore-rx/`](../../hardware/contenitore-rx/README.md) (CadQuery, STL e STEP).
- Dentro: SERVO42D e riduttore fissati con la **squadra a 90°**, che sta tra i due (riduttore davanti, motore
  dietro), con la base verso il riduttore e il motore su un appoggio, a una piastra interna (asole per viti M4),
  **sull'asse centrale** della scatola e su un rialzo di 20 mm (il variabile ha l'asse 20 mm più alto del motore:
  così i due alberi restano concentrici); due strisce di millefori 35×90 ai lati del motore (ESP32 DevKit da una
  parte, SN65HVD230 e convertitore 24→5 V dall'altra); OLED 0,96" sotto il coperchio, dietro un vetrino.
- L'albero esce al centro della parete frontale: foro per il **tubo isolante da 8 mm**, con paraolio 8×16×5.
- **Fissaggio al palo:** staffa stampata con due blocchi a V (pali da circa 20 a 50 mm) e due fascette stringitubo
  inox, avvitata al fondo con 4 inserti M4 ciechi. Palo verticale, parete frontale in alto verso il variabile,
  24 V e sfiato in basso.
- Gruppo giunto, riduttore, squadra e motore lungo circa 150 mm (misura dell'utente). Ingombro di circa 195 × 141 × 100 mm più il coperchio e la staffa (32 mm); guarnizione piatta in TPU, 8 viti M3
  con inserti a caldo, 1 PG7 (24 V), valvola di sfiato M12.
- **Finecorsa** (il variabile lavora su circa 180°): due sensori ottici a forcella (schedina 35 × 11 mm, fori a 28 mm)
  su un supporto stampato sotto l'albero, a 50° l'uno dall'altro, e una ruota con due bandierine sull'albero
  d'uscita del riduttore; il sensore B scatta 170° dopo il sensore A (parametro `CORSA`). Uscita alta con la luce
  interrotta, collegati agli ingressi dei finecorsa del SERVO42D.
- **Proposta:** il variabile va in una scatola separata, lontano dall'elettronica, invece di stare nella scatola
  unica descritta sotto. Da confermare.
- Tutte le misure dei componenti sono **stime da verificare** con il calibro (tabella nel README).

## Scatola stagna in plastica
- IP65 o superiore, circa 25×15×10 cm (variabile, riduttore, motore, SERVO42D).
- Variabile su distanziali in nylon, lontano da pareti e motore. Piattine di rame corte e larghe verso gli statori.
- Passacavi PG stagni per alimentazione, segnali e coassiale (ferriti all'esterno).
  I due capi dell'anello entrano da fori con gommino/manicotto in silicone; tappi sulle estremità dei tubi.
- **Condensa**: sacchetti di silica gel e una **valvola di sfiato a membrana** (tipo Gore).
- Motore libero (driver disabilitato) dopo ogni spostamento.
