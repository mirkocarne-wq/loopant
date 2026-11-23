# Contenitore stampabile dell'unità antenna RX

Scatola stagna da stampare in 3D. Contiene:
- il **SERVO42D** (NEMA17 con il driver sul retro) con il **riduttore planetario 20:1**, fissati con la loro
  **squadra a 90°** alla piastra interna;
- l'**elettronica** su millefori 90×70: ESP32 DevKit 30 pin, SN65HVD230, convertitore 24→5 V;
- il **display OLED 0,96"** sotto il coperchio, dietro una finestra con vetrino.

Il variabile da 1000 pF resta in una **scatola separata**, lontano dall'elettronica. L'asta in ABS da 8 mm esce
dalla parete frontale attraverso un paraolio e arriva al variabile.

![Interno](anteprime/montaggio.png)
![Esploso](anteprime/esploso.png)

Altre viste: [chiuso](anteprime/chiuso.png), [dal retro](anteprime/montaggio-retro.png),
[coperchio da dentro](anteprime/coperchio-interno.png).

## File
| File | Cosa |
|---|---|
| `contenitore.py` | Modello parametrico (CadQuery): tutte le misure sono in cima al file |
| `anteprime.py` | Immagini PNG (rasterizzatore in numpy, senza OpenGL) |
| `stl/` | Pezzi già orientati per la stampa |
| `step/` | Stessi pezzi in STEP, da aprire in FreeCAD, Fusion o nel programma di slicing |
| `anteprime/` | Immagini dell'insieme con i componenti finti |

Per rigenerare dopo aver cambiato le misure:
```bash
python3 -m venv ~/cq && ~/cq/bin/pip install cadquery matplotlib
```
```bash
~/cq/bin/python contenitore.py
```
Lo script controlla anche gli ingombri e avvisa se motore o elettronica toccano pareti, bordo o piastra.

## Misure (stime da verificare)
Ingombro esterno: circa **176 × 141 × 83 mm** più il coperchio (5,5 mm con la guarnizione); con le alette
171 mm di larghezza. Interno: 170 × 135 × 80 mm.

Le misure dei componenti sono **stime da verificare con il calibro** prima di stampare. Sono tutte variabili in
`contenitore.py`:

| Variabile | Valore | Cosa misurare |
|---|---|---|
| `H_ASSE` | 30 | Altezza dell'asse del riduttore sopra la base della squadra |
| `SQUADRA_BASE`, `ASOLE_SQUADRA` | 35; due asole a ±13 mm dall'asse | Fori della base della squadra (le asole lunghe 20 mm danno margine lungo l'asse) |
| `RIDUTTORE_L`, `RIDUTTORE_D` | 40, 36 | Corpo del riduttore |
| `MOTORE_L`, `DRIVER_L` | 40, 22 | NEMA17 e guscio del SERVO42D; dietro restano 27 mm per i connettori |
| `ALBERO_L`, `GIUNTO_L`, `GIUNTO_D` | 20, 25, 20 | Albero d'uscita (8 mm) e giunto verso l'asta ABS |
| `X_SQUADRA` | 38 | Distanza tra la squadra e la parete frontale: deve starci il giunto |
| `MILLEFORI`, `MILLEFORI_FORI` | 90×70, fori a 2,5 mm dal bordo | Millefori e fori di fissaggio (viti M2 autofilettanti) |
| `OLED_FORI`, `OLED_DY_PCB`, `OLED_VETRO` | 23×23,5; 1,5; 1,8 | Interassi dei fori dell'OLED; spostamento della zona attiva; spessore del vetro |
| `PARAOLIO_D`, `PARAOLIO_P` | 16,2; 5 | Paraolio 8×16×5 |

## Pezzi e stampa
| Pezzo | Materiale | Orientamento | Note |
|---|---|---|---|
| `scatola.stl` | **ASA** (o PETG) | Fondo sul piatto | Il bordo interno ha lo smusso a 45°: nessun supporto. 4 perimetri, 25–30 % di riempimento |
| `coperchio.stl` | ASA o PETG | Faccia esterna sul piatto (la scritta è incisa sotto) | Nessun supporto |
| `piastra.stl` | PETG | Piatta | 3 perimetri |
| `guarnizione.stl` | **TPU 85A–95A** | Piatta | In alternativa: foglio di silicone o EPDM da 1,5 mm, ritagliato usando lo STL come dima |

Il PLA non va bene sul balcone: al sole si deforma e con i raggi UV diventa fragile. Con il PETG conviene una
mano di vernice.

## Materiale
- 8 inserti a caldo M3 (Ø 4,0 mm, lunghi 5–6 mm) per il coperchio e 4 per la piastra; 8 viti M3×12 e 4 viti M3×8.
- Viti M4 con dadi e rondelle per la squadra: i dadi stanno sotto la piastra, che è rialzata di 6 mm.
- 4 viti M2×6 autofilettanti per la millefori e 4 per l'OLED.
- 2 pressacavi **PG7**: 24 V sul retro e cavo dei finecorsa sul fronte, verso la scatola del variabile.
- Valvola di sfiato a membrana **M12×1,5** sul retro, più un sacchetto di silica gel.
- **Paraolio 8×16×5** nella borchia frontale, con il labbro verso l'esterno; un velo di grasso al silicone sull'asta.
- Vetrino in plexiglass 31×17 mm, spesso 1,5 mm, incollato da dentro con silicone neutro.
- Viti M5 per le 4 alette (o fascette metalliche per il palo).

## Montaggio
1. Inserti a caldo nel bordo (8) e nelle colonnine del fondo (4).
2. Squadra con il SERVO42D e il riduttore sulla piastra, nelle asole; millefori sui distanziali.
3. Piastra nella scatola. L'apertura del bordo è 160×125 mm e la piastra 158×123 mm.
4. Giunto sull'albero del riduttore. Asta ABS infilata dall'esterno attraverso il paraolio, fino al giunto.
5. Cavi nei PG7 e fascette nelle fessure della piastra. OLED sulle colonnine del coperchio, vetrino incollato.
6. Coperchio con la guarnizione; viti strette in modo uniforme, a croce.

## Da verificare
- Tutte le misure della tabella sopra, prima della stampa definitiva. Si può stampare prima solo la piastra
  per provare la squadra e la millefori.
- Tenuta reale della guarnizione in TPU (prova con la doccia) e del paraolio sull'asta in ABS.
- Disturbo dell'OLED sui 40 e 20 m: è un punto aperto anche del firmware.
- Temperatura al sole d'estate. Se serve, coperchio chiaro o tettuccio.
