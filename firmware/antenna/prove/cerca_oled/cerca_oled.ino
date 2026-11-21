// Cerca un display SSD1306 (0x3C/0x3D) su tutte le coppie di GPIO libere dell'ESP32 e, se lo trova,
// accende un disegno di prova (fascia gialla piena, scacchiera nella zona blu).
#include <Wire.h>

const int PIN[] = {4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const int N = sizeof(PIN) / sizeof(PIN[0]);

void cmd(uint8_t a, uint8_t c) {
  Wire.beginTransmission(a); Wire.write(0x00); Wire.write(c); Wire.endTransmission();
}

void prova(uint8_t a) {
  const uint8_t init[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
                          0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x21, 0, 127, 0x22, 0, 7, 0xAF};
  for (uint8_t c : init) cmd(a, c);
  for (int pag = 0; pag < 8; pag++)
    for (int x = 0; x < 128; x += 16) {
      Wire.beginTransmission(a); Wire.write(0x40);
      for (int i = 0; i < 16; i++) Wire.write(pag < 2 ? 0xFF : (((x + i) / 8 + pag) % 2 ? 0xF0 : 0x0F));
      Wire.endTransmission();
    }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n== cerca_oled ==");
  int trovati = 0;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      if (i == j) continue;
      Wire.begin(PIN[i], PIN[j], 100000);
      // indirizzo di controllo 0x50: se risponde anche lui la linea SDA è bloccata bassa (falso positivo)
      Wire.beginTransmission(0x50);
      bool bloccata = Wire.endTransmission() == 0;
      for (uint8_t a = 0x3C; a <= 0x3D && !bloccata; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
          Serial.printf("TROVATO: indirizzo 0x%02X, SDA GPIO %d, SCL GPIO %d\n", a, PIN[i], PIN[j]);
          prova(a);
          trovati++;
        }
      }
      Wire.end();
      // sblocca il bus: 9 impulsi sul pin provato come SCL, con SDA libera
      pinMode(PIN[i], INPUT_PULLUP);
      pinMode(PIN[j], OUTPUT_OPEN_DRAIN);
      for (int k = 0; k < 9; k++) { digitalWrite(PIN[j], LOW); delayMicroseconds(5); digitalWrite(PIN[j], HIGH); delayMicroseconds(5); }
      pinMode(PIN[j], INPUT);
    }
  Serial.printf("== fine, trovati %d ==\n", trovati);
}

void loop() {}
