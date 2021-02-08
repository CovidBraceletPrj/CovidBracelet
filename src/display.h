#pragma once

#include <stdint.h>

#ifdef PLATFORM_PINETIME
#include "display_pinetime.h"
#define platform_display_init pinetime_display_init
#define platform_display_set_brightness pinetime_display_set_brightness
#else
static inline int platform_display_init() { return 0; }
static inline void platform_display_set_brightness(uint8_t brightness) { }
#endif