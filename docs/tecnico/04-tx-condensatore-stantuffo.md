# 04 — Versione TX: condensatore a stantuffo (tubo in tubo, PTFE)

## Principio
- Un tubo di rame interno scorre in un tubo esterno, con un **manicotto in PTFE** in mezzo.
  L'accoppiamento è capacitivo: **nessun contatto strisciante**.
- Capacità circa 7 pF/cm (PTFE 2 mm) o circa 9 pF/cm (PTFE 1,5 mm).
  Perdita nel dielettrico circa 6 W a 100 W / 40 m (stima).
- 1/f² è lineare con la posizione: bastano due parametri di calibrazione per banda.

## Dimensioni (progetto base)
| Parte | Misura |
|---|---|
| Tubo esterno | rame 28×1 (interno 26 mm), circa 20–22 cm |
| Pistone (tubo interno) | rame 22×1, circa 22 cm |
| Manicotto PTFE | interno 22 mm, esterno 25 mm (parete 1,5 mm) |
| Asta isolante | 8 mm, vetroresina o nylon, circa 16 cm |
| **Corsa** | **150 mm** (servono circa 11–14 cm per coprire 20 m–40 m) |

## Alternativa con tubi più facili da trovare
- **Pistone 18×1 ed esterno 22×1** (rame da idraulica comune): interno del 22×1 = 20 mm,
  quindi PTFE interno 18 / esterno 20 mm (parete 1 mm).
- Stima circa **11 pF/cm**, corsa necessaria circa 9–10 cm.

## Tappi e dettagli
- **Pistone**: tappo = disco di rame di circa 20 mm saldato **dentro** l'estremità che entra nel tubo esterno
  (un tappo a saldare commerciale va sopra il tubo e allargherebbe il diametro).
  Cordone lisciato, spigolo arrotondato (campo elevato).
- **Tubo esterno**: tappo normale a saldare sull'estremità di fondo, con **foro di sfiato da 2 mm** rivolto verso il basso
  (evita l'effetto pistone e fa uscire umidità/acqua).
- **Distanza minima**: a pistone tutto dentro, almeno **10 mm** tra il suo tappo e il fondo del tubo esterno,
  con un fermo meccanico (4–5 kV di picco).
- Il PTFE deve **sporgere** dal tubo esterno di almeno 1–2 cm (scarica sul bordo).
- Pulizia: lima, carta fine e alcol; nessuna sbavatura.
- Il PTFE non si incolla: bloccarlo nel tubo esterno con anelli o spine in nylon.

## Collegamenti all'anello
Disegno in sezione con collegamenti e dettagli: [`hardware/condensatore-tx/disegno-condensatore-tx.png`](../../hardware/condensatore-tx/disegno-condensatore-tx.png)
(script `disegno.py`, anche in SVG).

- **Tubo esterno** (fisso): collare di rame o ottone stretto sul tubo, verso il fondo e lontano dalla bocca, oppure
  linguetta saldata; piattina di rame larga e corta (20–25 × 1 mm) fino al primo capo dell'anello.
  Corrente RF stimata 9–13 A a 100 W: niente filo sottile.
- **Pistone** (mobile, 150 mm di corsa): **treccia piatta di rame stagnato** a trefoli fini, circa 20 mm di larghezza
  (10–16 mm²), fissata alla coda del pistone (l'estremità fuori dal tubo esterno, dove si attacca l'asta isolante)
  con un collare di rame o saldata; all'altro capo dell'anello con un'ansa a U più lunga della corsa (circa 16–18 cm).
- La treccia è allo stesso potenziale del pistone: deve stare **lontana dalla bocca del tubo esterno** (almeno 2–3 cm),
  dove ci sono 4–5 kV di picco. L'asta isolante spinge dietro l'attacco della treccia: il motore resta fuori dalla RF.
- La treccia si piega a ogni sintonia: controllarla ogni tanto (affaticamento dei trefoli).
- Scartato il pistone "flottante" tra due tubi esterni (nessun collegamento mobile): con circa 7 pF/cm la parte fissa
  dovrebbe essere lunga circa 70 cm per non limitare la capacità.

## Rondelle in PTFE da 2 mm (già acquistate)
- Non sostituiscono il manicotto: impilate, ogni giunto è una fessura d'aria tra pistone e tubo esterno, proprio dove
  la tensione è più alta.
- Usi utili: fermo isolante sul fondo del tubo esterno (5 rondelle = i 10 mm minimi), fermi del manicotto ai capi del
  tubo esterno, distanziali isolanti sotto le staffe. Diametri da verificare (interno almeno 22, esterno al massimo 26
  per entrare nel 28×1).

## Materiali
- Rame consigliato (resistenza più bassa). Ottone liscio e nudo accettabile
  (perdita stimata circa 0,7 W per tubo contro circa 0,35 W); mix rame/ottone senza problemi di corrosione.
- **Tubi per tende**: spesso acciaio o alluminio con finitura "ottone" → controllare con una calamita
  e misurare con il calibro diametro e parete.
- Tubo prolunga doccia Leroy Merlin (ottone, circa 24 mm, filettatura 3/4", 22,46 €): **scartato**.
