# 05 — Tuning e calibrazione

Codice: `firmware/antenna/src/calibrazione.*`.

## Principio
- Per il condensatore a stantuffo 1/f² è lineare con la posizione (C ∝ inserimento);
  per il variabile a lamelle si assume lo stesso in prima approssimazione.
- Per ogni banda si salva una tabella di punti (posizione, frequenza), fino a 16.
  La posizione per una frequenza si ricava per **interpolazione lineare in 1/f²** tra i punti vicini
  (o estrapolando dal primo/ultimo tratto, sempre dentro i limiti della banda).
- Servono **almeno 2 punti** per banda. Con il condensatore a stantuffo 2 punti dovrebbero bastare;
  più punti correggono le non linearità.
- Tabelle salvate in NVS (flash), restano dopo lo spegnimento.

## Procedura con il NanoVNA
1. NanoVNA sull'anello di accoppiamento, misura S11.
2. `zero` per azzerare.
3. Con `jog <passi>` spostarsi finché il dip di S11 cade sulla frequenza voluta
   (i movimenti arrivano sempre dallo stesso verso, come in uso normale).
4. `cal <frequenza>` salva il punto. Ripetere ai due estremi della banda (meglio anche al centro).
5. Ripetere per l'altra banda. Annotare anche Q ≈ f0 / BW.
6. `tabella` mostra i punti salvati; `calcanc <frequenza>` cancella la tabella di quella banda.

## In uso
1. `GOTO freq` → se la banda è cambiata, azzeramento sul finecorsa.
2. Posizione dalla tabella, arrivo dallo stesso verso.
3. **Ricerca fine del picco** attorno alla posizione di tabella — **da implementare**, serve un sensore:
   - RX: segnale audio/S-meter (AGC manuale, senza attenuatore);
   - TX: ROS minimo a bassa potenza (circa 5 W), poi blocco e piena potenza.
4. **Mai muovere il condensatore con RF ad alta potenza.**
