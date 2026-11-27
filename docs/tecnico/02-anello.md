# 02 — Anello

## Versione RX
- **Diametro 1 m** (raggio 50 cm sull'asse del tubo), lunghezza di tubo circa **3,3 m**
  (3,14 m di circonferenza + margine per i capi e i tagli).
- **Tubo di rame**: **Ø 20 mm** (già preso dall'utente; prima si pensava a 1/2" o 5/8" da frigorista). Stima da
  verificare: L circa 2,5 µH, quindi circa 820 pF a 3,5 MHz e circa 50 pF a 14 MHz; peso circa 1,8 kg con parete 1 mm.
  Rispetto ai 18 mm il 5/8" perde circa 0,5 dB in più, l'1/2" circa 1,5 dB, il 3/8" circa 2,8 dB.
  In ricezione la differenza è trascurabile.
- Rotolo da 5 m. Curvatura con sagoma circolare (compensato, tavolo rotondo, hula hoop da 1 m), molla piegatubi o sabbia.
  Piegare su un raggio di circa 48 cm per compensare il ritorno elastico.
- Giunzioni a stagno o raccordi a compressione ben serrati. Le resistenze di contatto contano più del diametro del tubo.

## Versione TX
- Anello 1,5 m, tubo **22 mm**. Il rame è la scelta migliore (corrente 9–13 A).

## Anello di accoppiamento
- Circa 1/5 del diametro (circa 20 cm), in cavo coassiale RG58/RG213 (circa 70 cm), con ferriti sul coassiale.
- **Il coassiale va solo al loop di accoppiamento**; l'anello grande non è collegato al cavo: con il condensatore è
  un circuito risonante e scambia il segnale con il loop piccolo per accoppiamento magnetico. I due capi dell'anello
  vanno ai due terminali del condensatore (RX: statore e telaio, o i due statori; TX: tubo esterno e pistone).
- Schema dei collegamenti RF, circuito equivalente e cablaggio del loop piccolo (in tubetto o schermato in coassiale):
  [`hardware/collegamenti-rf/schema-rf.png`](../../hardware/collegamenti-rf/schema-rf.png).

## Posizionamento sul balcone
- Centro dell'anello ad almeno 1,2–1,5 m dal pavimento, ad almeno mezzo diametro da pareti e ringhiere metalliche.
  Palo in vetroresina/PVC.
- Scatola del condensatore in basso, vicino al palo; anello di accoppiamento in alto.
- Si può ruotare l'anello per annullare un disturbo (nullo nel piano).
- In TX a 100 W: persone e animali a più di 2 m.
