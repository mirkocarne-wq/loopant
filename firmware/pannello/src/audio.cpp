#include "audio.h"

#include <Arduino.h>
#include <esp_adc/adc_continuous.h>
#include <math.h>

#include <Decoder.h>

namespace audio {
namespace {

constexpr size_t BYTE_FRAME = 256 * SOC_ADC_DIGI_RESULT_BYTES;  // 256 campioni dell'ADC per lettura
constexpr float TAGLIO_HZ = 3600;  // -6 dB del FIR: piatto fino a circa 2,8 kHz (toni RTTY fino a 2,3 kHz)

adc_continuous_handle_t adc = nullptr;
adc_channel_t canale;
Consumatore consumatore = nullptr;
volatile bool attivo_ = false;
volatile float livello_ = 0;
uint32_t fsAdc = 32000;
decoder::Decimatore decimatore;

void compito(void*) {
  static uint8_t grezzi[BYTE_FRAME];
  static int16_t campioni[BYTE_FRAME / SOC_ADC_DIGI_RESULT_BYTES];
  float media = 2048;  // componente continua (metà scala)
  float energia = 0;
  const float alfa = 8.0f / fsAdc;  // passa-alto molto lento (costante di tempo circa 0,125 s)
  for (;;) {
    uint32_t letti = 0;
    if (adc_continuous_read(adc, grezzi, sizeof(grezzi), &letti, 1000) != ESP_OK) continue;
    size_t n = 0;
    for (uint32_t i = 0; i + SOC_ADC_DIGI_RESULT_BYTES <= letti; i += SOC_ADC_DIGI_RESULT_BYTES) {
      const adc_digi_output_data_t* d = (const adc_digi_output_data_t*)&grezzi[i];
      if (d->type2.channel != canale) continue;
      float v = (float)d->type2.data;
      media += (v - media) * alfa;  // toglie la continua
      float y;
      if (!decimatore.campione(v - media, y)) continue;
      float c = y * 16.0f;  // 12 bit -> 16 bit (la media sui 4 campioni dà circa un bit in più)
      if (c > 32767) c = 32767;
      if (c < -32768) c = -32768;
      campioni[n++] = (int16_t)c;
      energia += (c * c - energia) * 0.001f;
    }
    livello_ = sqrtf(energia) / 32768.0f;
    if (n && consumatore) consumatore(campioni, n);
  }
}

}  // namespace

bool inizia(int gpio, uint32_t fs, Consumatore c) {
  if (gpio < 0 || attivo_) return attivo_;
  adc_unit_t unita;
  if (adc_continuous_io_to_channel(gpio, &unita, &canale) != ESP_OK || unita != ADC_UNIT_1) return false;

  adc_continuous_handle_cfg_t hc = {};
  hc.max_store_buf_size = 4 * BYTE_FRAME;
  hc.conv_frame_size = BYTE_FRAME;
  if (adc_continuous_new_handle(&hc, &adc) != ESP_OK) return false;

  adc_digi_pattern_config_t schema = {};
  schema.atten = ADC_ATTEN_DB_12;  // circa 0-3,1 V
  schema.channel = canale;
  schema.unit = ADC_UNIT_1;
  schema.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
  adc_continuous_config_t cfg = {};
  cfg.pattern_num = 1;
  cfg.adc_pattern = &schema;
  fsAdc = fs * decoder::Decimatore::FATTORE;
  decimatore.configura((float)fsAdc, TAGLIO_HZ);
  cfg.sample_freq_hz = fsAdc;
  cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
  cfg.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;
  if (adc_continuous_config(adc, &cfg) != ESP_OK || adc_continuous_start(adc) != ESP_OK) {
    adc_continuous_deinit(adc);
    adc = nullptr;
    return false;
  }
  consumatore = c;
  attivo_ = true;
  xTaskCreatePinnedToCore(compito, "audio", 6144, nullptr, 3, nullptr, 0);
  return true;
}

bool attivo() { return attivo_; }

float livello() { return livello_; }

}  // namespace audio
