# 12 — Manopola remota

Scatoletta remota vicino alla radio: **decoder CW/RTTY dal microfono**, **ritocco fine** dell'accordo con la manopola,
**frequenza e TUNE**, display rotondo con testo e livello. Codice: `firmware/manopola/`.
Legenda: **[D]** = deciso; **[N]** = proposta da confermare.

## Hardware [D]
**Waveshare ESP32-S3-Knob-Touch-LCD-1.8** (ESP32-S3R8, 8 MB PSRAM, 16 MB flash, USB-C nativa; sulla scheda c'è anche
un ESP32 classico, non usato). Pin dallo schema Waveshare:

| Funzione | GPIO | Note |
|---|---|---|
| Microfono MEMS PDM MSM261D4030H1CPM | CLK 45, DATI 46 | canale sinistro |
| Manopola (encoder) | A 8, B 7 | driver knob di Espressif (`bidi_switch_knob`) |
| Display SH8601 360×360 QSPI | CS 14, CLK 13, D0-D3 15-18, RST 21, retroilluminazione 47 | ruotato di 180° (MADCTL 0xC0) |
| Touch CST816 | SDA 11, SCL 12 | coordinate invertite per la rotazione |
| DAC PCM5100A (uscita cuffie) | 39, 40, 41 | non usato |

- Niente line-in sulla scheda (il jack è un'uscita): l'ingresso è il **microfono**, davanti all'altoparlante della
  radio. Un PCM1808 si potrebbe collegare solo ai pin della microSD (GPIO 2-6, 42), saldando: non previsto.
- Provato il 21/11/2026: display, rotazione, touch, manopola nei due versi, microfono.

## Architettura [D]
La manopola **passa sempre dal pannello**: è il pannello a comandare l'antenna (un solo "capo", niente conflitti).
- Manopola → pannello (`MANOPOLA_STATO`, 10 volte al secondo): stato del decoder, ultimi caratteri decodificati con
  un contatore (il pannello prende quelli nuovi anche se perde pacchetti), contatore degli scatti, richieste di TUNE.
- Pannello → manopola (`MANOPOLA_IMPOSTA`, ogni secondo, vale anche come heartbeat): impostazioni del decoder (quelle
  della pagina Decoder del pannello), sorgente, dati dell'antenna per il display, passi per scatto.
- Scatti → `JOG` di (scatti × passi per scatto), solo ad antenna ferma e azzerata; TUNE → `GOTO`.
- Associazione come l'antenna (finestra «Manopola trovata» sul pannello), ma **senza pulsante**: finché non è
  associata la manopola cerca il pannello da sola ogni 15 s. Poi ritrova il canale da sola se il router lo cambia.
- Provato il 21/11/2026: associazione e testo del segnale di prova sul pannello e sulla manopola.

## Uso [D]
| Gesto | Effetto |
|---|---|
| Girare la manopola (RITOCCO FINE) | sposta il condensatore di N passi per scatto (N in Impostazioni → Manopola remota) |
| Tocco breve sul display | passa tra RITOCCO FINE e FREQUENZA |
| Girare la manopola (FREQUENZA) | 1 kHz per scatto, 10 kHz girando in fretta; oltre il bordo della banda si passa all'altra |
| Tocco lungo (FREQUENZA) | TUNE sulla frequenza mostrata, poi si torna al ritocco fine |

Display: modo e collegamento in alto, al centro posizione (passi) o frequenza a 7 segmenti, frequenza accordata,
ultime lettere decodificate e CW WPM / RTTY agganciato; il livello audio è l'arco sul bordo.

## Audio e decoder [D]
Microfono PDM a 32 kHz, componente continua tolta, FIR e riduzione a 8 kHz (`Decoder::Decimatore`, come il
pannello), decoder CW e RTTY della libreria `firmware/lib/Decoder`. Sorgente «Segnale di prova»: CW o RTTY sintetici
generati sulla manopola (`generatore.*`), per provare senza la radio.

## Pannello
- Con la manopola associata la pagina Decoder mostra testo e livelli della manopola («Manopola · microfono ·
  audio %»); l'ingresso J8 del pannello non si usa.
- Impostazioni → **Manopola remota**: stato del collegamento, sorgente (Microfono, Segnale di prova), passi per scatto
  (5-500, predefinito 20).
- Codice: `link_manopola.*`, `decodifica.*`, `ui/finestra_associazione.cpp`, `ui/pagina_impostazioni.cpp`.

## Console della manopola (USB, 115200)
`stato` · `associa` · `dimentica` (dimentica il pannello) · `canale <1-13>`

## Da fare
- Provare ritocco fine e TUNE con l'antenna collegata. [N]
- Vibrazione a ogni scatto con il motore aptico DRV2605 della scheda. [N]
- Livello del microfono e posizione della manopola rispetto all'altoparlante: da provare con la radio. [N]
