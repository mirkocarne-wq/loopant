# 09 — Propagazione (pannello)

Pagina **Propagazione** e screensaver **Grey line**. Requisiti P1–P3 e V3 in [03-unita-controllo.md](03-unita-controllo.md).
Legenda: **[D]** = deciso; **[N]** = proposta tecnica, da confermare.

## Calcolato sul pannello, senza Internet [D]
Serve solo l'ora UTC esatta e la posizione della stazione (dal locator).

- **Declinazione del sole**: δ ≈ −23,44° · cos(2π/365 · (N + 10)), con N = giorno dell'anno.
- **Equazione del tempo** (minuti): B = 2π(N − 81)/365; EoT ≈ 9,87·sin 2B − 7,53·cos B − 1,5·sin B.
- **Longitudine subsolare**: λs = −15° · (UTC − 12 + EoT/60).
- **Terminatore** (grey line): per ogni longitudine λ, latitudine φ = atan(−cos(λ − λs) / tan δ).
  Con δ < 0 la notte è a nord della curva, con δ > 0 a sud.
- **Alba e tramonto alla stazione**: cos H0 = (sin(−0,833°) − sin φ · sin δ) / (cos φ · cos δ);
  mezzogiorno UTC = 12 − λ/15 − EoT/60; alba/tramonto = mezzogiorno ∓ H0/15.
- Precisione: qualche minuto, sufficiente per la grey line (da confrontare con un calcolatore di riferimento).

## Mappa [N]
- Proiezione equirettangolare (2:1), per esempio 600×300 nella pagina e 1024×512 nello screensaver.
- Nel mockup i continenti sono poligoni molto semplificati. Nel firmware: immagine della mappa in flash
  (dati Natural Earth, pubblico dominio), una per tema, con la notte disegnata sopra.
- Città di riferimento: New York, Los Angeles, Rio de Janeiro, Città del Capo, Mosca, Nuova Delhi, Tokyo, Sydney.

## Dati via WiFi [D]
- Indici solari **SFI, SSN, A, K** e stima delle condizioni di banda giorno/notte nelle quattro fasce del feed:
  80–40, 30–20, 17–15 e 12–10 m (dal 26/11/2026; prima solo le prime due).
- Fonte proposta: il feed XML di hamqsl.com (N0NBH), aggiornato ogni 1–3 ore. [N] Da verificare formato e condizioni d'uso.
- Il WiFi del pannello si collega al router di casa: **il canale ESP-NOW diventa quello del router**,
  da impostare anche sull'antenna (requisito E7).
- Senza WiFi la pagina mostra la mappa e alba/tramonto; gli indici appaiono come non disponibili.

## Ora esatta [N]
- Con il WiFi: NTP all'avvio e periodicamente.
- Senza WiFi: impostazione manuale (il 7B non documenta un RTC); in futuro dalla radio via CI-V.

## Realizzazione (21/11/2026)
Codice: `firmware/pannello/src/sole.*`, `locator.*`, `mappa.*`, `indici.*`, `rete.*`, `ui/pagina_propagazione.cpp`.
- La mappa si disegna in PSRAM (riempimento per righe di scansione: i continenti non sono convessi), poi a ogni minuto
  si aggiungono notte, grey line (fascia di circa 3° intorno al terminatore), sole e stazione.
- Il locator (4 o 6 caratteri) dà la posizione della stazione.
- Prova sul PC: alba 06:08 e tramonto 15:44 UTC a Roma il 20/11/2026 (riferimento indicativo 06:05 / 15:44).
- Il WiFi è un task sul core 0: NTP all'avvio della connessione, indici ogni 2 ore (o a richiesta da Impostazioni).
