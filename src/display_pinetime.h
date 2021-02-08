#pragma once

#include <stdint.h>
#include <stdbool.h>

int pinetime_display_init();
void pinetime_display_set_brightness(uint8_t brightness);
void pinetime_display_draw_string(uint8_t x, uint8_t y, const char *str);
void pinetime_display_set_powersave(bool powersave);
