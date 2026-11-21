// Configurazione dell'unità antenna: pin, meccanica, velocità, tempi.
// Pinout: docs/tecnico/06-elettronica.md. I valori "da verificare" vanno confermati sul banco.
#pragma once
#include <stdint.h>

#ifndef VERSIONE_TX
#define VERSIONE_TX 1  // 1 = TX (NEMA11 + TMC2209), 0 = RX (SERVO42D STEP/DIR/EN)
#endif

namespace cfg {

// --- Pin (ESP32 DevKit 30 pin, tutti presenti anche sulla 38 pin; evitare 0, 2, 12, 15;
//     analogici solo ADC1 32-39) ---
constexpr int PIN_STEP = 18;
constexpr int PIN_DIR = 19;
constexpr int PIN_EN = 23;     // attivo basso (TMC2209 e SERVO42D); libero all'accensione, niente pull interne
constexpr int PIN_TMC_TX = 25;  // UART TMC2209 (con resistenza da 1 kΩ verso PDN_UART)
constexpr int PIN_TMC_RX = 26;
constexpr int PIN_FC_MIN = 27;  // finecorsa lato zero
constexpr int PIN_FC_MAX = 14;  // finecorsa lato opposto
constexpr bool FC_ATTIVO_BASSO = true;  // microswitch verso massa, pull-up + RC 100 nF

// CAN verso il SERVO42D (versione RX, alternativa a STEP/DIR): transceiver Waveshare SN65HVD230.
// Il driver (STEP/DIR o CAN) si sceglie dal pannello e resta salvato in NVS.
constexpr int PIN_CAN_TX = 17;  // TX2 -> CTX del transceiver
constexpr int PIN_CAN_RX = 16;  // RX2 -> CRX del transceiver
constexpr uint16_t CAN_ID_PREDEFINITO = 1;      // indirizzo del SERVO42D (menu del driver)
constexpr uint16_t CAN_KBIT_PREDEFINITO = 500;  // deve essere uguale a quello del SERVO42D
// Comandi del SERVO42D (protocollo da verificare sul manuale ufficiale V1.0.3)
constexpr uint8_t SERVO_MODO_CAN = 5;       // SR_vFOC: comandi dal bus
constexpr uint8_t SERVO_MODO_IMPULSI = 2;   // CR_vFOC: STEP/DIR
constexpr uint8_t SERVO_ACCELERAZIONE = 2;  // 0-255 (0 = senza rampa)
constexpr uint8_t SERVO_HOME_LIVELLO = 0;   // finecorsa attivo basso (da verificare)
constexpr uint8_t SERVO_HOME_VERSO = 1;     // 0 = CW, 1 = CCW verso il finecorsa (da verificare)
constexpr uint16_t SERVO_HOME_RPM = 60;
constexpr uint32_t SERVO_TIMEOUT_MS = 250;  // attesa della risposta prima di ripetere

// Display OLED 0,96" (SSD1306 128x64, I2C 0x3C, prime 16 righe gialle) sui pin I2C standard dell'ESP32.
// Facoltativo: se non risponde il firmware funziona senza.
constexpr int PIN_OLED_SDA = 21;
constexpr int PIN_OLED_SCL = 22;
constexpr uint8_t OLED_INDIRIZZO = 0x3C;
// Spegnimento dopo questi secondi senza cambi di stato né comandi (0 = sempre acceso): il convertitore
// interno del display potrebbe disturbare la ricezione, come il driver del motore (da verificare con la radio).
constexpr uint16_t OLED_SPEGNI_S = 120;

// --- Collegamento ---
constexpr uint8_t CANALE_WIFI = 1;  // predefinito; si cambia dalla console con "canale <n>" (NVS)
constexpr uint32_t TIMEOUT_HEARTBEAT_MS = 3000;  // 3 heartbeat persi
// Ricerca del canale (il router lo cambia e il pannello lo segue): se il pannello tace da SILENZIO, a motore fermo,
// l'antenna prova i canali 1-13 con un messaggio di stato che la radio del pannello conferma (ACK).
// Se non lo trova (pannello spento) riprova ogni INTERVALLO.
constexpr uint32_t RICERCA_CANALE_SILENZIO_MS = 5000;
constexpr uint32_t RICERCA_CANALE_INTERVALLO_MS = 10000;
constexpr uint32_t RICERCA_CANALE_ACK_MS = 40;  // attesa della conferma su ogni canale (2 tentativi)

#if VERSIONE_TX
// --- TX: NEMA11 + vite T6x1 (1 mm/giro), corsa 150 mm ---
constexpr uint16_t MICROPASSI = 16;
constexpr int32_t PASSI_PER_GIRO = 200 * MICROPASSI;  // 3200
constexpr int32_t PASSI_PER_MM = PASSI_PER_GIRO;      // vite da 1 mm/giro
constexpr int32_t CORSA_PASSI = 150 * PASSI_PER_MM;   // 480000

constexpr float R_SENSE = 0.11f;      // tipico dei moduli TMC2209: da verificare
constexpr uint8_t TMC_INDIRIZZO = 0;  // MS1/MS2 a massa
constexpr uint16_t CORRENTE_MA = 500;  // RMS, da verificare sulla scheda del NEMA11 (0,5-1 A)
constexpr uint16_t SOGLIA_BLOCCO = 0;  // SG_RESULT minimo; 0 = StallGuard disabilitato (da tarare)

constexpr uint32_t VELOCITA_HZ = 5 * PASSI_PER_GIRO;              // 300 giri/min
constexpr uint32_t VELOCITA_AZZERAMENTO_HZ = 3 * PASSI_PER_GIRO;  // ricerca veloce
constexpr uint32_t VELOCITA_LENTA_HZ = PASSI_PER_GIRO / 4;        // riavvicinamento lento
constexpr uint32_t ACCELERAZIONE = 4 * PASSI_PER_GIRO;            // passi/s²

constexpr int32_t STACCO_PASSI = 1 * PASSI_PER_MM;               // 1 mm dal finecorsa
constexpr int32_t RECUPERO_GIOCO_PASSI = PASSI_PER_MM / 2;       // 0,5 mm oltre il target
#else
// --- RX: SERVO42D (micropassi impostati sul driver) + riduttore 20:1 sul variabile ---
// Rotazione utile del variabile circa 180°: da misurare.
constexpr uint16_t MICROPASSI = 16;                         // come impostato sul SERVO42D
constexpr int32_t PASSI_PER_GIRO = 200 * MICROPASSI * 20;   // per giro dell'albero del variabile
constexpr int32_t CORSA_PASSI = PASSI_PER_GIRO / 2;         // 180°: da misurare

constexpr uint32_t VELOCITA_HZ = 2 * 200 * MICROPASSI;  // 2 giri/s del motore
constexpr uint32_t VELOCITA_AZZERAMENTO_HZ = VELOCITA_HZ;
constexpr uint32_t VELOCITA_LENTA_HZ = 200 * MICROPASSI / 4;
constexpr uint32_t ACCELERAZIONE = 4 * 200 * MICROPASSI;

constexpr int32_t STACCO_PASSI = PASSI_PER_GIRO / 180;          // circa 2°
constexpr int32_t RECUPERO_GIOCO_PASSI = PASSI_PER_GIRO / 360;  // circa 1°, > gioco di 0,5°
#endif

// Limiti software del target
constexpr int32_t POS_MIN = STACCO_PASSI + RECUPERO_GIOCO_PASSI;
constexpr int32_t POS_MAX = CORSA_PASSI - STACCO_PASSI;
// Oltre questa distanza senza trovare il finecorsa l'azzeramento fallisce
constexpr int32_t AZZERAMENTO_MAX_PASSI = CORSA_PASSI + CORSA_PASSI / 10;

}  // namespace cfg
