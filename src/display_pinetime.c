#ifdef PLATFORM_PINETIME

#include <zephyr.h>
#include <device.h>
#include <display/cfb.h>
#include <drivers/gpio.h>

#include <u8x8.h>

#include "display_pinetime.h"

#define BACKLIGHT_LOW_OF_NODE DT_ALIAS(led0)
#define BACKLIGHT_MID_OF_NODE DT_ALIAS(led1)
#define BACKLIGHT_HI_OF_NODE  DT_ALIAS(led2)
// dev and flags are the same for all backlight GPIOs
#define BACKLIGHT_DEVNAME DT_GPIO_LABEL(BACKLIGHT_LOW_OF_NODE, gpios)
#define BACKLIGHT_FLAGS   (DT_GPIO_FLAGS(BACKLIGHT_LOW_OF_NODE, gpios) | GPIO_OUTPUT)
#define BACKLIGHT_LOW_PIN DT_GPIO_PIN(BACKLIGHT_LOW_OF_NODE, gpios)
#define BACKLIGHT_MID_PIN DT_GPIO_PIN(BACKLIGHT_MID_OF_NODE, gpios)
#define BACKLIGHT_HI_PIN  DT_GPIO_PIN(BACKLIGHT_HI_OF_NODE, gpios)

static struct {
    const struct device *display_dev;
    const struct device *backlight_dev;
} pinetime_display;

static const u8x8_display_info_t pinetime_display_info = {
    // Tile setup for u8x8
    .tile_width = 240 / 8,
    .tile_height = 240 / 8,
    // Pixel setup for u8g2
    .pixel_width = 240,
    .pixel_height = 240,
};

static uint16_t tile_tmp[8 * 8];
// Represents a single u8x8 tile
const struct display_buffer_descriptor buffer_desc = {
    .buf_size = sizeof(tile_tmp),
    .width = 8,
    .height = 8,
    .pitch = 8
};

uint8_t u8x8_cmd_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    u8x8_tile_t *tile;
    int i, x, y;

    switch(msg) {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
        u8x8_d_helper_display_setup_memory(u8x8, &pinetime_display_info);
        break;
    case U8X8_MSG_DISPLAY_INIT:
	    u8x8_d_helper_display_init(u8x8);
        break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
        tile = ((u8x8_tile_t *)arg_ptr);
        uint8_t *tile_ptr = tile->tile_ptr;

        for (i = 0; i < tile->cnt; i++) {
            int rep = 0;

            for (x = 0; x < 8; x++) {
                uint8_t byte = *tile_ptr++;

                for (y = 0; y < 8; y++) {
                    tile_tmp[x + y * 8] = (byte & (1 << y)) ? 0xffff : 0;
                }
            }
            while (rep < arg_int) {
                display_write(pinetime_display.display_dev, (tile->x_pos + i + rep) * 8, tile->y_pos * 8, &buffer_desc, tile_tmp);
                rep++;
            }
        }
        break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
        if (arg_int) {
            display_blanking_on(pinetime_display.display_dev);
        } else {
            display_blanking_off(pinetime_display.display_dev);
        }
    }
    return 1;
}

static u8x8_t u8x8;

void pinetime_display_set_brightness(const uint8_t brightness) {
    // Parse 3 MSB only
    gpio_pin_set(pinetime_display.backlight_dev, BACKLIGHT_HI_PIN,  brightness & 0x80);
    gpio_pin_set(pinetime_display.backlight_dev, BACKLIGHT_MID_PIN, brightness & 0x40);
    gpio_pin_set(pinetime_display.backlight_dev, BACKLIGHT_LOW_PIN, brightness & 0x20);
}

int pinetime_display_init() {
    int err;
 
    // Get display device
    pinetime_display.display_dev = device_get_binding("ST7789V");
    if (!pinetime_display.display_dev) {
        return -ENODEV;
    }

    pinetime_display.backlight_dev = device_get_binding(BACKLIGHT_DEVNAME);
    if (!pinetime_display.backlight_dev) {
        return -ENODEV;
    }

    // Initialize backlight GPIOs
    err = gpio_pin_configure(pinetime_display.backlight_dev, BACKLIGHT_LOW_PIN, BACKLIGHT_FLAGS);
    if (err) {
        return err;
    }

    err = gpio_pin_configure(pinetime_display.backlight_dev, BACKLIGHT_MID_PIN, BACKLIGHT_FLAGS);
    if (err) {
        return err;
    }
 
    err = gpio_pin_configure(pinetime_display.backlight_dev, BACKLIGHT_HI_PIN, BACKLIGHT_FLAGS);
    if (err) {
        return err;
    }
    pinetime_display_set_brightness(0);

    // Set up graphics library
    // Populate all hardware callbacks with dummy functions, we do already have a driver
    u8x8_Setup(&u8x8, u8x8_cmd_cb, u8x8_dummy_cb, u8x8_dummy_cb, u8x8_dummy_cb);
    u8x8_InitDisplay(&u8x8);
    u8x8_ClearDisplay(&u8x8);
    u8x8_SetPowerSave(&u8x8, true);
    u8x8_SetFont(&u8x8, u8x8_font_amstrad_cpc_extended_f);

    return 0;
}

void pinetime_display_draw_string(uint8_t x, uint8_t y, const char *str) {
    u8x8_DrawString(&u8x8, x, y, str);
}

void pinetime_display_set_powersave(bool powersave) {
    u8x8_SetPowerSave(&u8x8, false);
    if (powersave) {
        pinetime_display_set_brightness(0);
    }
}

#endif