# 03 — Versione RX: condensatore e meccanica

## Condensatore
- Variabile ad aria 1000 pF "1000V CC" (Jieotwice, Amazon).
- Da misurare con il NanoVNA: capacità minima e massima, **continuità albero–telaio** (probabile contatto strisciante).
- Bande 80–20 m con il variabile da solo (circa 45–800 pF, tabella in [01](01-obiettivo-principi.md)). Per i 20 m
  servono circa 50 pF: se il minimo vero, con le parassite, è più alto, serve C1 in serie anche lì.

### Misure del variabile montato (NanoVNA, 28/11/2026)
Posizioni in passi del pannello con il SERVO42D com'è oggi (MStep da controllare: circa 2100 passi per tutta
l'escursione, il firmware ne prevede circa 32000 con 16 micropassi). Lo zero è dalla parte della capacità minima.

| Passi | 0 | 50 | 500 | 600 | 700 | 1000 | 1500 | 1900 | 2028 |
|---|---|---|---|---|---|---|---|---|---|
| pF | 32,6 | 32,7 | 200 | 230 | 280 | 430 | 676 | 875 | 919 |

- Minimo misurato 32,6 pF (non 20), massimo circa 920 pF vicino al finecorsa: con L tra 2,6 e 4,2 µH (anello
  misurato 4,2 µH, da ricontrollare: la formula dà circa 2,6) l'anello va da circa 2,6–3,2 MHz a circa 12–15 MHz
  (stime da verificare con la risonanza). Con 4,2 µH i 20 m non si raggiungono col solo variabile.
- Prima risonanza: 4,155 MHz a 1132 passi (circa 495 pF): L circa 2,9 µH compresi i collegamenti (stima, da
  confermare con un secondo punto). Con poche parassite il variabile da solo copre circa 3,05–14,3 MHz.
- Circa 0,47 pF per passo nel tratto lineare: troppo grosso per centrare la risonanza sui 20 m (MStep 16 lo porta
  a circa 0,03 pF per passo).

## Condensatori in serie per 17–10 m (proposta del 26/11/2026, da costruire)
Sotto i circa 28 pF il variabile non scende (minimo circa 20 pF più le parassite). Con un condensatore fisso in serie
la capacità totale è C·Cv/(C + Cv): resta sotto C qualunque sia la posizione del variabile.

```
 anello ──┬── C1 ──┬── C2 ──┬── variabile ── anello
          └─ K1 ───┘└─ K2 ──┘
```
- **K1 e K2 a riposo** (contatti chiusi, C1 e C2 in corto): variabile da solo, 80–20 m.
- **K1 eccitato** (C1 in serie): 17 e 15 m. Con C1 circa 33 pF il totale va da circa 14,5 a 34 pF.
- **K1 e K2 eccitati** (C1 e C2 in serie, con C2 circa 27 pF: circa 15 pF): 12 e 10 m, totale da circa 10,5 a
  16,5 pF. La parte alta dei 10 m (29,5–29,7 MHz) potrebbe restare appena fuori.
- Valori **stimati** (L 2,6–2,8 µH, 2 pF di parassite, variabile lineare): C1 e C2 vanno scelti dopo aver misurato
  con il NanoVNA il minimo vero del variabile e le parassite. Comodi due compensatori (trimmer) da circa 5–40 pF
  per tararli sul posto, poi eventualmente fissi a mica o NP0. La combinazione per banda si cambia dalla console
  (`serie <banda> <0-3>`), senza ricaricare il firmware.
- Relè **nella scatola del variabile**, sui collegamenti più corti possibile: piccoli relè di segnale o reed a 5 V
  con poca capacità tra i contatti aperti. In RX la tensione sul condensatore è bassa.
- I fili delle bobine arrivano dall'elettronica: impedenze (ferrite) e condensatori di fuga a 100 nF sulle due
  estremità, perché l'albero è isolato apposta (porta RF) e i fili non devono riportarla nella scatola del motore.
- Comandati dall'ESP32 (GPIO 4 e 13) con un transistor e un diodo di ricircolo ciascuno ([06](06-elettronica.md)).
- Con C1/C2 inseriti anche la tabella di calibrazione è un'altra: ogni banda ha la sua, e il firmware salva un punto
  solo se i relè sono nella combinazione di quella banda.

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
  **sull'asse centrale** della scatola e su un rialzo di 20 mm più 2 mm di spessore sotto la squadra (asse comune delle due scatole a 60 mm dal fondo interno:
  così i due alberi restano concentrici); due strisce di millefori 35×90 ai lati del motore (ESP32 DevKit da una
  parte, SN65HVD230 e convertitore 24→5 V dall'altra); OLED 0,96" sotto il coperchio, dietro un vetrino.
- L'albero esce al centro della parete frontale: foro per il **tubo isolante da 8 mm**, con paraolio 8×16×5.
- **Fissaggio al palo:** staffa stampata con due blocchi a V (pali da circa 20 a 50 mm) e due fascette stringitubo
  inox, avvitata al fondo con 4 inserti M4 ciechi. Palo verticale, parete frontale in alto verso il variabile,
  24 V e sfiato in basso.
- Gruppo giunto, riduttore, squadra e motore lungo circa 150 mm (misura dell'utente). Ingombro di circa 195 × 141 × 100 mm più il coperchio e la staffa (32 mm); guarnizione piatta in TPU, 8 viti M3
  con inserti a caldo, 1 PG7 (24 V), valvola di sfiato M12.
- **Finecorsa** (escursione del variabile 190° da fermo a fermo): due sensori ottici a forcella (schedina 35 × 11 mm, fori a 28 mm)
  su un supporto stampato sotto l'albero, a 50° l'uno dall'altro, e una ruota con due bandierine sull'albero
  d'uscita del riduttore; i sensori scattano 5° prima di ciascun fermo, quindi B scatta 180° dopo A (parametri `ESCURSIONE`, `MARGINE`). Uscita alta con la luce
  interrotta, collegati agli ingressi dei finecorsa del SERVO42D.
- **Proposta:** il variabile va in una scatola separata, lontano dall'elettronica, invece di stare nella scatola
  unica descritta sotto. Da confermare.
- Tutte le misure dei componenti sono **stime da verificare** con il calibro (tabella nel README).

## Scatola del variabile (stampata)
- `hardware/contenitore-rx/scatola_variabile.py`: stessa costruzione della scatola del motore (fondo, staffa,
  coperchio, borchia con paraolio), interno 105 mm di larghezza; sul palo sta sopra la scatola del motore.
- Variabile 20–1000 pF, 135 × 83 × 95 mm, 600 g, fissato alla piastra con 2 viti M5 dal basso
  nelle linguette di sotto (fori filettati M5 a 114 mm); asse 45 mm sopra la base del variabile, piastra su colonnine da 11 mm, a 60 mm dal fondo
  interno come il motore: alberi concentrici.
- **Regge l'anello**: i capi del tubo di rame entrano dall'alto in un blocco pieno di 20 mm, bloccati da una vite M4
  di traverso per capo; la staffa del palo si allunga sotto il blocco con 2 viti in più.
- Collegamento dei capi: piattina di rame 12 × 1 mm saldata in un taglio nella punta del tubo (al banco, prima di
  infilarlo nel blocco); con un solo statore un capo allo statore e l'altro al telaio (corrente nei cuscinetti, da
  verificare), con due statori isolati un capo per statore.
- Sostituisce, se confermata, la scatola unica descritta sotto.

## Assieme
- `hardware/contenitore-rx/assieme.py`: palo, scatola del motore, scatola del variabile sopra, anello da 1 m con i
  capi nel blocco della scatola del variabile; cima dell'anello a circa 1,52 m dal fondo della scatola del motore.
- Alimentazione: coassiale solo sul loop di accoppiamento (20 cm, in cima dentro l'anello); l'anello grande è accoppiato magneticamente.
- Proposta: cima stampata (`cima.py`): raccordo in due metà (T, braccio Ø 40 e tappo) che stringe la cima del palo in PVC da 40, con una vite M5 attraverso il palo; sostegno con la scatoletta dell'SO-239 in punta al braccio.

## Scatola stagna in plastica
- IP65 o superiore, circa 25×15×10 cm (variabile, riduttore, motore, SERVO42D).
- Variabile su distanziali in nylon, lontano da pareti e motore. Piattine di rame corte e larghe verso gli statori.
- Passacavi PG stagni per alimentazione, segnali e coassiale (ferriti all'esterno).
  I due capi dell'anello entrano da fori con gommino/manicotto in silicone; tappi sulle estremità dei tubi.
- **Condensa**: sacchetti di silica gel e una **valvola di sfiato a membrana** (tipo Gore).
- Motore libero (driver disabilitato) dopo ogni spostamento.
