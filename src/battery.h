#pragma once

#include <stdint.h>

int battery_init();
int battery_update();
uint16_t battery_get_voltage_mv();
uint32_t battery_voltage_mv_to_soc(uint16_t batt_mv);