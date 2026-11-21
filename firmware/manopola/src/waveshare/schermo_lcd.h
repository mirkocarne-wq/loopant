// Display QSPI SH8601 360x360, touch CST816 e LVGL della manopola (driver Waveshare).
#pragma once
#include "Arduino.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_lvgl_Init(void);                // display, touch e LVGL (chiamare dopo Touch_Init)
bool example_lvgl_lock(int timeout_ms);  // -1 = attesa infinita
void example_lvgl_unlock(void);

#ifdef __cplusplus
}
#endif
