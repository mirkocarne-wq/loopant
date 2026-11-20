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
| Rilevazione del tono | Algoritmo di Goertzel centrato sul tono (predefinito 700 Hz, regolabile) | [N] |
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

## Note di realizzazione [N]
- La decodifica gira su un task FreeRTOS dedicato (core 0); LVGL sul core 1, come negli esempi Waveshare.
- Parametri del decoder salvati in NVS.
- Il decoder è solo ricezione: il pannello non trasmette.
