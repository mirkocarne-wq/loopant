# 07 — Sviluppi futuri

## Interfaccia con la radio
- Icom **CI-V** o Yaesu **CAT**: lasciare libero un UART sulla scheda di controllo.
- Funzione "segui la radio": legge la frequenza e porta l'antenna in risonanza.

## Wattmetro / ROS (TX)
- Accoppiatore direzionale + due AD8307 + ADS1115 letto dall'ESP32.
- Soglie di allarme; misura del ROS per la ricerca fine del picco a bassa potenza.

## Sensori di posizione
- Sensore lineare KTC-150 o encoder magnetico AS5600 se si perdono passi.
  Ingressi analogici solo su ADC1 (GPIO 32–39).

## Pairing
- Scambio dei MAC in broadcast premendo un pulsante, al posto dell'impostazione manuale.
