#include "microfono.h"

#include <Arduino.h>
#include <Decoder.h>
#include <driver/i2s_pdm.h>
#include <math.h>

#include "config.h"

namespace microfono {
namespace {

constexpr float TAGLIO_HZ = 3600;  // come il pannello: piatto fino a circa 2,8 kHz
constexpr size_t BLOCCO = 256;     // campioni del microfono per lettura

i2s_chan_handle_t canale = nullptr;
Consumatore consumatore = nullptr;
volatile bool attivo_ = false;
volatile float livello_ = 0;
uint32_t fsMic = 32000;
decoder::Decimatore decimatore;

void compito(void*) {
  static int16_t grezzi[BLOCCO];
  static int16_t campioni[BLOCCO / decoder::Decimatore::FATTORE + 1];
  float media = 0, energia = 0;
  const float alfa = 8.0f / fsMic;  // passa-alto molto lento (costante di tempo circa 0,125 s)
  for (;;) {
    size_t letti = 0;
    if (i2s_channel_read(canale, grezzi, sizeof(grezzi), &letti, 1000) != ESP_OK) continue;
    size_t n = 0;
    for (size_t i = 0; i < letti / sizeof(int16_t); i++) {
      float v = grezzi[i];
      media += (v - media) * alfa;
      float y;
      if (!decimatore.campione(v - media, y)) continue;
      if (y > 32767) y = 32767;
      if (y < -32768) y = -32768;
      campioni[n++] = (int16_t)y;
      energia += (y * y - energia) * 0.001f;
    }
    livello_ = sqrtf(energia) / 32768.0f;
    if (n && consumatore) consumatore(campioni, n);
  }
}

}  // namespace

bool inizia(uint32_t fs, Consumatore c) {
  if (attivo_) return true;
  fsMic = fs * decoder::Decimatore::FATTORE;
  decimatore.configura((float)fsMic, TAGLIO_HZ);
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  if (i2s_new_channel(&cc, nullptr, &canale) != ESP_OK) return false;
  i2s_pdm_rx_config_t pc = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(fsMic),
      .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg = {.clk = (gpio_num_t)cfg::PIN_MIC_CLK, .din = (gpio_num_t)cfg::PIN_MIC_DATI, .invert_flags = {.clk_inv = false}},
  };
  if (i2s_channel_init_pdm_rx_mode(canale, &pc) != ESP_OK || i2s_channel_enable(canale) != ESP_OK) {
    i2s_del_channel(canale);
    canale = nullptr;
    return false;
  }
  consumatore = c;
  attivo_ = true;
  xTaskCreatePinnedToCore(compito, "microfono", 6144, nullptr, 3, nullptr, 0);
  return true;
}

bool attivo() { return attivo_; }

float livello() { return livello_; }

}  // namespace microfono
