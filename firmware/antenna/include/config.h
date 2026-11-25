// Configurazione dell'unità antenna: pin, meccanica, velocità, tempi.
// Pinout: docs/tecnico/06-elettronica.md. I valori "da verificare" vanno confermati sul banco.
#pragma once
#include <stdint.h>

namespace cfg {

// --- Pin (ESP32 DevKit 30 pin, tutti presenti anche sulla 38 pin; evitare 0, 2, 12, 15;
//     analogici solo ADC1 32-39) ---
// STEP/DIR/EN, UART e finecorsa: solo antenna TX (TMC2209)
constexpr int PIN_STEP = 18;
constexpr int PIN_DIR = 19;
constexpr int PIN_EN = 23;     // attivo basso (TMC2209 e SERVO42D); libero all'accensione, niente pull interne
constexpr int PIN_TMC_TX = 25;  // UART TMC2209 (con resistenza da 1 kΩ verso PDN_UART)
constexpr int PIN_TMC_RX = 26;
constexpr int PIN_FC_MIN = 27;  // finecorsa lato zero
constexpr int PIN_FC_MAX = 14;  // finecorsa lato opposto
constexpr bool FC_ATTIVO_BASSO = true;  // microswitch verso massa, pull-up + RC 100 nF

// CAN verso il SERVO42D (antenna RX): transceiver Waveshare SN65HVD230. Finecorsa collegati al SERVO42D.
constexpr int PIN_CAN_TX = 17;  // TX2 -> CTX del transceiver
constexpr int PIN_CAN_RX = 16;  // RX2 -> CRX del transceiver
constexpr uint16_t CAN_ID_PREDEFINITO = 1;      // indirizzo del SERVO42D (menu del driver)
constexpr uint16_t CAN_KBIT_PREDEFINITO = 500;  // deve essere uguale a quello del SERVO42D
// Comandi del SERVO42D (manuale CAN V1.0.9; CRC, 0xFE, 0x91, 0xF1, 0x31, 0x8C ancora da confrontare)
constexpr uint8_t SERVO_MODO_CAN = 5;       // SR_vFOC: comandi dal bus
constexpr uint8_t SERVO_MODO_IMPULSI = 2;   // CR_vFOC: STEP/DIR
constexpr uint8_t SERVO_ACCELERAZIONE = 2;  // 0-255 (0 = senza rampa)
// Parametri di zero (0x90). Il livello lo impone sempre il firmware; verso e velocità solo se il SERVO42D non li
// fa leggere (firmware prima di V1.0.6), altrimenti restano quelli del suo menu (HmDir, HmSpeed).
// Finecorsa ottici a forcella alimentati da EVCC/EGND, uscita su En (zero) e Dir (limite CW), COM a EVCC: con la
// bandierina nella forcella danno 1 nella lettura 0x34 (provato a mano il 25/11/2026), quindi HmTrig High.
constexpr uint8_t SERVO_HOME_LIVELLO = 1;
constexpr uint8_t SERVO_HOME_VERSO = 1;     // 0 = CW, 1 = CCW: lo zero è sul lato CCW (HmDir invertito il 21/11/2026)
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

// --- Meccanica ---
// Firmware unico: il tipo di antenna (TX o RX) si sceglie dal pannello e resta in NVS (tipo_antenna.h).
struct Meccanica {
  uint16_t micropassi;
  int32_t passiPerGiro;            // per giro dell'albero mosso (vite o variabile)
  int32_t corsaPassi;
  uint32_t velocitaHz;
  uint32_t velocitaAzzeramentoHz;  // ricerca veloce del finecorsa
  uint32_t velocitaLentaHz;        // riavvicinamento lento
  uint32_t accelerazione;          // passi/s²
  int32_t staccoPassi;             // distanza dal finecorsa dopo l'azzeramento
  int32_t recuperoGiocoPassi;      // oltre il target, per arrivare sempre dal verso crescente

  // Limiti software del target, contati dal finecorsa (posMin: spazio per il recupero del gioco sotto il target).
  // Fuori da movimento.cpp le posizioni contano da dove finisce l'azzeramento.
  constexpr int32_t posMin() const { return staccoPassi + recuperoGiocoPassi; }
  constexpr int32_t posMax() const { return corsaPassi - staccoPassi; }
  // Oltre questa distanza senza trovare il finecorsa l'azzeramento fallisce
  constexpr int32_t azzeramentoMaxPassi() const { return corsaPassi + corsaPassi / 10; }
};

// TX: NEMA11 + vite T6x1 (1 mm/giro), corsa 150 mm. 200 passi x 16 micropassi = 3200 passi/giro = 3200 passi/mm.
constexpr int32_t TX_PASSI_PER_MM = 200 * 16;
constexpr Meccanica TX = {
    16,                    // micropassi (impostati via UART)
    TX_PASSI_PER_MM,       // passi per giro della vite
    150 * TX_PASSI_PER_MM,  // corsa 150 mm = 480000 passi
    5 * TX_PASSI_PER_MM,   // 300 giri/min (corsa completa circa 30 s)
    3 * TX_PASSI_PER_MM,
    TX_PASSI_PER_MM / 4,
    4 * TX_PASSI_PER_MM,
    TX_PASSI_PER_MM,       // 1 mm dal finecorsa
    TX_PASSI_PER_MM / 2,   // 0,5 mm oltre il target
};

// RX: SERVO42D (micropassi impostati sul driver) + riduttore 20:1 sul variabile.
// Rotazione utile del variabile circa 180°: da misurare.
constexpr int32_t RX_PASSI_MOTORE = 200 * 16;  // per giro del motore, come impostato sul SERVO42D
constexpr Meccanica RX = {
    16,
    RX_PASSI_MOTORE * 20,       // per giro dell'albero del variabile
    RX_PASSI_MOTORE * 20 / 2,   // 180°: da misurare
    2 * RX_PASSI_MOTORE,        // 2 giri/s del motore
    2 * RX_PASSI_MOTORE,
    RX_PASSI_MOTORE / 4,
    4 * RX_PASSI_MOTORE,
    RX_PASSI_MOTORE * 20 / 180,  // circa 2°
    RX_PASSI_MOTORE * 20 / 360,  // circa 1°, più del gioco di 0,5°
};

// Tipo al primo avvio (NVS vuota): RX, che ha corsa e velocità minori (più prudente).
constexpr bool TIPO_TX_PREDEFINITO = false;

// Solo TX: TMC2209 via UART
constexpr float R_SENSE = 0.11f;      // tipico dei moduli TMC2209: da verificare
constexpr uint8_t TMC_INDIRIZZO = 0;  // MS1/MS2 a massa
constexpr uint16_t CORRENTE_MA = 500;  // RMS, da verificare sulla scheda del NEMA11 (0,5-1 A)
constexpr uint16_t SOGLIA_BLOCCO = 0;  // SG_RESULT minimo; 0 = StallGuard disabilitato (da tarare)

}  // namespace cfg
