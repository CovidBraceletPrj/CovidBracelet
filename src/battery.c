/*
 * Copyright (c) 2021 Tobias Schramm <t.schramm@t-sys.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/adc.h>

#include "battery.h"

#define VBATT DT_PATH(vbatt)
#define BATTERY_ADC_GAIN ADC_GAIN_1_6
#define BATTERY DT_PATH(battery)

static struct {
    const struct device *adc;
    uint32_t output_resistance_ohm;
    uint32_t full_resistance_ohm;
    struct adc_sequence sequence;
    uint16_t raw_value;
    uint16_t value_mv;
} battery_adc_config;

int battery_init() {
    struct adc_channel_cfg config;

    battery_adc_config.adc = device_get_binding(DT_IO_CHANNELS_LABEL(VBATT));
    if (!battery_adc_config.adc) {
        return -ENODEV;
    }

    battery_adc_config.output_resistance_ohm = DT_PROP(VBATT, output_ohms);
    battery_adc_config.full_resistance_ohm = DT_PROP(VBATT, full_ohms);

    battery_adc_config.sequence.channels = BIT(0);
    battery_adc_config.sequence.buffer = &battery_adc_config.raw_value;
    battery_adc_config.sequence.buffer_size = sizeof(battery_adc_config.raw_value);
    battery_adc_config.sequence.oversampling = 4;
    battery_adc_config.sequence.calibrate = true;
    battery_adc_config.sequence.resolution = 14;

    config.gain = BATTERY_ADC_GAIN;
    config.reference = ADC_REF_INTERNAL;
    config.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40);
    config.channel_id = 0;
    config.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + DT_IO_CHANNELS_INPUT(VBATT);

    return adc_channel_setup(battery_adc_config.adc, &config);
}

int battery_update() {
    int err;
    int32_t val_mv;

    err = adc_read(battery_adc_config.adc, &battery_adc_config.sequence);
    if (err) {
        return err;
    }

    battery_adc_config.sequence.calibrate = false;

    val_mv = battery_adc_config.raw_value;
    err = adc_raw_to_millivolts(adc_ref_internal(battery_adc_config.adc), BATTERY_ADC_GAIN, battery_adc_config.sequence.resolution, &val_mv);
    if (err) {
        return err;
    }

    battery_adc_config.value_mv = val_mv * battery_adc_config.full_resistance_ohm / battery_adc_config.output_resistance_ohm;
    return 0;
}

uint16_t battery_get_voltage_mv() {
    return battery_adc_config.value_mv;
}

const uint32_t battery_soc_voltage_mv_pairs[] = DT_PROP(BATTERY, soc_voltage_mv);

#if DT_PROP_LEN(BATTERY, soc_voltage_mv) % 2 != 0
#error length of soc_voltage_mv must be divisible by two!
#endif

uint32_t battery_voltage_mv_to_soc(uint16_t batt_mv) {
    int i = 0;
    uint32_t last_soc;
    uint32_t last_voltage_mv;

    if (!ARRAY_SIZE(battery_soc_voltage_mv_pairs)) {
        return 100;
    }

    last_soc = battery_soc_voltage_mv_pairs[0];
    last_voltage_mv = battery_soc_voltage_mv_pairs[1];

    if (batt_mv >= last_voltage_mv) {
        return last_soc;
    }

    for (i = 1; i < ARRAY_SIZE(battery_soc_voltage_mv_pairs) / 2; i++) {
        uint32_t soc = battery_soc_voltage_mv_pairs[i * 2];
        uint32_t voltage_mv = battery_soc_voltage_mv_pairs[i * 2 + 1];

        if (batt_mv >= voltage_mv && batt_mv <= last_voltage_mv) {
            uint32_t delta_mv = last_voltage_mv - voltage_mv;
            uint32_t delta_soc = last_soc - soc;
            uint32_t frac_mv = batt_mv - voltage_mv;

            return soc + frac_mv * delta_soc / delta_mv;
        }

        last_soc = soc;
        last_voltage_mv = voltage_mv;
    }

    return last_soc;
}