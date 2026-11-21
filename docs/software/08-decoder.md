# 08 — Decoder CW e RTTY (pannello)

Pagina **Decoder** del pannello in casa. Requisiti D1–D3 in [03-unita-controllo.md](03-unita-controllo.md).
Legenda: **[D]** = deciso; **[N]** = proposta tecnica, da confermare.

## Ingresso audio
- Sorgente: **uscita linea / audio della radio** (Malahit DSP2, in futuro IC-7300 MK2). [D]
- Il 7B **non documenta un ingresso audio**. Servirà un piccolo modulo esterno: [N]
  - ADC I2S (es. un modulo con PCM1808), oppure
  - partitore + condensatore di disaccoppiamento verso un ingresso analogico libero (connettore "sensore", pin da verificare sullo schema).
- Campionamento proposto: 8 kHz, 16 bit (sufficiente per toni fino a circa 3 kHz). [N]
- Isolamento dalla radio (trasformatore audio 1:1) per evitare loop di massa e rumore. [N]

## CW
| Funzione | Specifica | |
|---|---|---|
| Rilevazione del tono | Algoritmo di Goertzel centrato sul tono (predefinito 700 Hz, regolabile a passi di 10 Hz) | [D] |
| Filtro | Larghezza 100 / 250 / 500 Hz (durata del blocco di Goertzel) | [D] |
| Soglia | Adattiva, mostrata sulla barra del livello | [D] |
| Velocità | Stima automatica in WPM dalla durata dei punti; possibilità di fissarla a mano | [D] |
| Decodifica | Rapporti di tempo punto/linea/spazi (1:3:7) → tabella Morse | [N] |
| Visualizzazione | Testo grande, ultimi elementi ricevuti (punti/linee), tono e WPM su display a 7 segmenti | [D] |

## RTTY
| Parametro | Valori | |
|---|---|---|
| Velocità | **45,45** / 50 / 75 baud | [D] |
| Shift | **170** / 425 / 850 Hz | [D] |
| Tono di mark | 2125 Hz predefinito, regolabile a passi di ±1 / ±10 Hz (300–3000 Hz); space = mark + shift | [D] |
| Bit di stop | 1 / **1,5** / 2 | [D] |
| Reverse | Inverte mark e space | [D] |
| UOS | Unshift on space (torna alle lettere dopo uno spazio) | [D] |
| AFC | Insegue la deriva del tono | [D] |
| Codice | Baudot ITA2, lettere/cifre | [N] |
| Demodulazione | Due filtri (Goertzel o passa-banda) su mark e space, confronto delle energie, campionamento al centro del bit | [N] |
| Indicatore di sintonia | Barre di livello mark e space, stato "agganciato" | [D] |

## Realizzazione (21/11/2026)
Codice: `firmware/pannello/src/decoder/` (algoritmi), `src/audio.*` (ADC), `src/decodifica.*`, `src/ui/pagina_decoder.cpp`.

- **Ingresso**: ADC continuo dell'ESP32-S3 a 8 kHz su `cfg::PIN_AUDIO` (GPIO6, da verificare), continua tolta con
  un passa-alto lento, task sul core 0. LVGL resta sul core 1.
- **CW**: Goertzel su blocchi con un numero intero di periodi del tono (altrimenti l'ampiezza oscilla con la fase);
  rumore stimato a tasto alzato, squelch a 6 volte il rumore, isteresi 55 %/35 %, conferma su 2 blocchi,
  breve riscaldamento iniziale; punto/linea a 2 dit, fine carattere a 2 dit, fine parola a 5 dit; WPM stimati
  (oppure fissi) tra 5 e 50.
- **RTTY**: miscelazione I/Q su mark e space con media mobile di 0,8 bit, confronto delle energie, start sul
  fronte mark→space, campionamento a metà bit, controllo dello stop; Baudot ITA2 con LTRS/FIGS e UOS;
  AFC dalla rotazione di fase del tono attivo (ogni 50 ms, ±100 Hz).
- **Prove sul PC** (`firmware/pannello/test/esegui_test.sh`): CW 12-28 WPM con filtri 100/250/500 Hz e rumore,
  RTTY 45,45/170, Reverse, 50/850, AFC con 25 Hz di errore, rumore forte; ripetute con 10 sequenze di rumore.
  **Da provare con segnali veri** dalla radio.
- Parametri salvati in NVS; il decoder è solo ricezione.
