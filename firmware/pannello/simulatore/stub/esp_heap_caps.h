#pragma once
// Anche per il C di LVGL (lv_conf.h): static inline vale in C e in C++.
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_8BIT 0
static inline void* heap_caps_malloc(size_t n, int) { return malloc(n); }
static inline void* heap_caps_realloc(void* p, size_t n, int) { return realloc(p, n); }
static inline void heap_caps_free(void* p) { free(p); }
