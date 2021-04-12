#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <sys/crc.h>

#include "ens_fs.h"

#define SEED 42

int ens_fs_init(ens_fs_t* fs, uint8_t id, uint64_t entry_size) {
    int rc = flash_area_open(id, &fs->area);
    if (rc) {
        return rc;
    }

    const struct device* dev = flash_area_get_device(fs->area);

    fs->sector_count = flash_get_page_count(dev);
    struct flash_pages_info info;
    rc = flash_get_page_info_by_offs(dev, fs->area->fa_off, &info);
    fs->sector_size = info.size;
    fs->entry_size = entry_size;
    return 0;
}

int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dist) {

    uint64_t offset = id * fs->entry_size;

    int rc = flash_area_read(fs->area, offset, dist, fs->entry_size);
    if(rc) {
       goto end;
    }

    // TODO lome: Read deleted flag and CRC
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

    // TODO lome: Maybe overwrite last 4 bytes? Or make it file system dependend?    
    // uint8_t data = 0;
    // uint64_t offset = (id + 1) * fs->entry_size - 1;

    // int rc = flash_area_write(fs->area, offset, &data, 0);
    // if(rc) {
    //     goto end;
    // }
    return 0;
}

int ens_fs_page_erase(ens_fs_t* fs, uint64_t offset, uint64_t sector_count) {
    uint64_t start = (offset - offset % fs->sector_size) * fs->entry_size;

    int rc = flash_area_erase(fs->area, start, fs->sector_size * sector_count);

    return rc;
}
