#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <sys/crc.h>
#include <string.h>

#include "ens_fs.h"

#define SEED 42

int ens_fs_init(ens_fs_t* fs, uint8_t id, uint64_t entry_size) {
    int rc = flash_area_open(id, &fs->area);
    if (rc) {
        goto end;
    }

    // Check, if entry size is multiple of flash_area_align
    if((entry_size % flash_area_align(fs->area)) != 0) {
        flash_area_close(fs->area);
        // TODO lome: set custom error code
        rc = -1;
        goto end;
    }
    fs->entry_size = entry_size;

    const struct device* dev = flash_area_get_device(fs->area);

    fs->sector_count = flash_get_page_count(dev);
    struct flash_pages_info info;
    rc = flash_get_page_info_by_offs(dev, fs->area->fa_off, &info);
    fs->sector_size = info.size;

end:
    return rc;
}

int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dist) {

    uint64_t offset = id * fs->entry_size;

    int rc = flash_area_read(fs->area, offset, dist, fs->entry_size);
    if(rc) {
       goto end;
    }

    uint8_t* obj = dist;
    uint8_t metadata = crc7_be(SEED, obj, fs->entry_size - 1) | 1;
    int isInvalid = memcmp(&obj[fs->entry_size - 1], &metadata, 1);
    if(isInvalid) {
        // If checksum is not equal to calculated checksum or if deleted flag is not 1, set memory to 0
        rc = -1;
        memset(dist, 0, fs->entry_size);
    }
end:

    return rc;
}

int ens_fs_write(ens_fs_t* fs, uint64_t id, void* data) {

    // Set CRC and not-deleted-flag
    uint8_t* obj = data;
    obj[fs->entry_size - 1] = crc7_be(SEED, obj, fs->entry_size - 1) | 1;

    uint64_t offset = id * fs->entry_size;
    int rc = flash_area_write(fs->area, offset, data, fs->entry_size);
    if(rc) {
        goto end;
    }

end:
    return rc;
}

int ens_fs_delete(ens_fs_t* fs, uint64_t id) {

    uint8_t data[fs->entry_size];

    uint64_t offset = id * fs->entry_size;
    int rc = flash_area_read(fs->area, offset, data, fs->entry_size);
    if(rc) {
        goto end;
    }

    // set memory to 0, so not-deleted flag is 0
    memset(data, 0, fs->entry_size);
    rc = flash_area_write(fs->area, offset, data, fs->entry_size);

end:
    return rc;
}

int ens_fs_page_erase(ens_fs_t* fs, uint64_t offset, uint64_t sector_count) {
    uint64_t start = (offset - offset % fs->sector_size) * fs->entry_size;

    int rc = flash_area_erase(fs->area, start, fs->sector_size * sector_count);

    return rc;
}
