#pragma once

#include <devicetree.h>
#include <stdint.h>

// Generic voltage divider based battery support
#if DT_NODE_HAS_STATUS(DT_PATH(vbatt), okay)
#define BATTERY_SUPPORTED
int battery_init();
int battery_update();
uint16_t battery_get_voltage_mv();
#endif

// SOC estimation support
#if DT_NODE_HAS_STATUS(DT_PATH(battery), okay)
#define BATTERY_SOC_SUPPORTED
uint32_t battery_voltage_mv_to_soc(uint16_t batt_mv);
#endif