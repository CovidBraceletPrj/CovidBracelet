#pragma once

#include <stdint.h>

int battery_init();
int battery_update();
uint16_t battery_get_voltage_mv();