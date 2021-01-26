#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "storage.h"

// Maybe use this as param for init function
#define SEC_COUNT 10

static struct nvs_fs fs;

int init_storage(void) {
    int rc = 0;
    struct flash_pages_info info;
    // define the nvs file system
    fs.offset = FLASH_AREA_OFFSET(storage);
    rc = flash_get_page_info_by_offs(device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL), fs.offset, &info);

    if (rc) {
        return rc;
    }
    fs.sector_size = info.size;
    fs.sector_count = SEC_COUNT;

    rc = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    if (rc) {
        return rc;
    }
}