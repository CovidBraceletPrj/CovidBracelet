#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_LINE_CURRENT_IDENTIFIER  0
#define DISPLAY_LINE_LAST_CONTACTS_START 2

#ifdef PLATFORM_PINETIME
#include "display_pinetime.h"
#define platform_display_init pinetime_display_init
#define platform_display_set_brightness pinetime_display_set_brightness
#define platform_display_draw_string pinetime_display_draw_string
#define platform_display_set_powersave pinetime_display_set_powersave
#else
static inline int platform_display_init() { return 0; }
static inline void platform_display_set_brightness(uint8_t brightness) { (void)brightness; }
static inline void platform_display_draw_string(uint8_t x, uint8_t y, const char *str) { (void)x; (void)y; (void)str; }
static inline void platform_display_set_powersave(bool powersave) { (void)powersave; }
#endif