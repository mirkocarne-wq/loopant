# 01 — Obiettivo e principi

## Obiettivo
- Antenna magnetic loop da balcone (circa 4 m): **RX dagli 80 ai 10 m**, **TX 40 e 20 m** (RX allargata dal
  26/11/2026, decisione dell'utente; prima 40 e 20 m per entrambe).
- **Due versioni**:
  1. **RX** (solo ricezione): anello 1 m, variabile a lamelle da 1000 pF già in possesso.
     Ricevitore: Malahit DSP2 (e in futuro Icom IC-7300 MK2).
  2. **TX 100 W**: anello 1,5 m, condensatore a "stantuffo" (tubo in tubo con PTFE), motorizzato, con wattmetro/ROS.
- Sintonia **automatica**: sull'ESP32 con touch in casa si digita la frequenza e l'antenna si porta in risonanza.

## Formule
- Risonanza: f = 1 / (2π√(LC)).
- Induttanza anello circolare: L ≈ μ0·(D/2)·(ln(8·(D/2)/a) − 2), con a = raggio del tubo.
- Tensione sul condensatore: Vc = √(P·Q·Xc). Corrente: I = √(P·Q/Xc).
- Banda molto stretta: circa 40–50 kHz sui 20 m, circa 28 kHz sui 40 m → va riaccordata a ogni cambio di frequenza.
- Pattern a "otto", perpendicolare al piano dell'anello; nulli nel piano.

## Induttanze stimate (anello 1 m, circolare)
| Tubo | Diametro esterno | L stimata |
|---|---|---|
| 3/8" | 9,5 mm | circa 3,0 µH |
| 1/2" | 12,7 mm | circa 2,8 µH |
| 5/8" | 15,9 mm | circa 2,65 µH |
| 18 mm | 18 mm | circa 2,6 µH |
| 3/4" | 19 mm | circa 2,55 µH |

## Capacità richieste (stime, con circa 4 pF parassiti)
- Anello **RX 1 m** (circa 2,6–2,8 µH): 40 m circa 180–195 pF, 20 m circa 45–50 pF. Tutte le bande RX (stime del
  26/11/2026, capacità totale con le parassite):

  | Banda | Capacità | Come |
  |---|---|---|
  | 80 m (3,5–3,8 MHz) | 630–800 pF | variabile da solo |
  | 60 m (5,35 MHz) | 315–340 pF | variabile da solo |
  | 40 m | 180–195 pF | variabile da solo |
  | 30 m (10,1 MHz) | 88–95 pF | variabile da solo |
  | 20 m | 45–48 pF | variabile da solo |
  | 17 m (18,1 MHz) | 28–30 pF | al limite del minimo del variabile: C1 in serie |
  | 15 m (21,2 MHz) | 20–22 pF | C1 in serie |
  | 12 m (24,9 MHz) | 15–16 pF | C1 e C2 in serie |
  | 10 m (28–29,7 MHz) | 10–12,5 pF | C1 e C2 in serie |

  Il variabile (20–1000 pF) arriva fino a circa 3 MHz; sotto il suo minimo servono i condensatori in serie
  ([03](03-rx-condensatore-meccanica.md)). Sui 10 m l'anello è circa 0,3 λ: non è più un loop piccolo (da verificare).
- Anello **TX 1,5 m, tubo 22 mm** (L circa 4 µH): 40 m circa 118–130 pF, 20 m circa 28–33 pF.
- Per il TX serve una escursione di circa **100 pF**.

## Stress del condensatore in TX 100 W (Q ≈ 300, stima)
- Circa 3,3 kV di picco sui 40 m, circa 4,7 kV di picco sui 20 m.
- Corrente circa 9–13 A.
- Il variabile a lamelle da 1000 pF (spaziatura circa 1 mm, "1000 V CC" dichiarati) **non è adatto al TX**; solo prove a 1–2 W.

## Rendimento stimato (da verificare)
- Anello DIY 1,5 m: circa 30% sui 40 m, circa 85% sui 20 m.
- Riferimento commerciale: Käferlein AMA-41 (0,8 m, 100 W): 21% a 7 MHz, 76% a 14 MHz.
