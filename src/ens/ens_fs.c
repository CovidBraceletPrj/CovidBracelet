#include <drivers/flash.h>
#include <kernel.h>
#include <storage/flash_map.h>
#include <string.h>
#include <sys/crc.h>

#include "ens_error.h"
#include "ens_fs.h"

#define SEED 42

// mask for metadata
#define CRC_MASK 254

#define GET_CHECKSUM(x) (x & CRC_MASK)

int ens_fs_init(ens_fs_t* fs, uint8_t flash_id, uint64_t entry_size) {
    if (flash_area_open(flash_id, &fs->area)) {
        // opening of flash area was not successful
        return -ENS_INTERR;
    }

    // check, if entry size is multiple of flash_area_align
    if ((entry_size % flash_area_align(fs->area)) != 0) {
        flash_area_close(fs->area);
        return -ENS_INVARG;
    }
    fs->entry_size = entry_size;

    // get all information needed for the needed for the fs
    const struct device* dev = flash_area_get_device(fs->area);
    fs->sector_count = flash_get_page_count(dev);
    struct flash_pages_info info;
    if (flash_get_page_info_by_offs(dev, fs->area->fa_off, &info)) {
        // on error, close the flash area
        flash_area_close(fs->area);
        return -ENS_INTERR;
    }
    fs->sector_size = info.size;

    k_mutex_init(&fs->ens_fs_lock);
    return 0;
}

int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dest) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // read the entry from flash
    uint64_t offset = id * fs->entry_size;

    if (flash_area_read(fs->area, offset, dest, fs->entry_size)) {
        // opening of flash area was not successful
        rc = -ENS_INTERR;
        goto end;
    }

    // check, if checksum and not-deleted flag are as expected
    uint8_t* obj = dest;

    int isNotDeleted = obj[fs->entry_size - 1] & 1;
    if (!isNotDeleted) {
        // entry got deleted
        rc = -ENS_DELENT;
        goto end;
    }

    // crc stored in entry
    uint8_t entryCRC = GET_CHECKSUM(obj[fs->entry_size - 1]);

    // calculated crc
    uint8_t checkCRC = crc7_be(SEED, obj, fs->entry_size - 1);

    int isInvalid = memcmp(&entryCRC, &checkCRC, 1);
    if (isInvalid) {
        // if checksum is not equal to calculated checksum or if deleted flag is not 1, set memory to 0
        memset(dest, 0, fs->entry_size);
        // we do not know, if object got deleted or data is corrupt
        rc = -ENS_NOENT;
    }

end:
    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_write(ens_fs_t* fs, uint64_t id, void* data) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // set CRC and not-deleted-flag
    uint8_t* obj = data;
    obj[fs->entry_size - 1] = crc7_be(SEED, obj, fs->entry_size - 1) | 1;

    uint64_t offset = id * fs->entry_size;
    if (flash_area_write(fs->area, offset, data, fs->entry_size)) {
        // writing to flash was not successful
        rc = -ENS_INTERR;
    }

    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_delete(ens_fs_t* fs, uint64_t id) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    uint8_t data[fs->entry_size];

    uint64_t offset = id * fs->entry_size;
    if (flash_area_read(fs->area, offset, data, fs->entry_size)) {
        // reading was not successful
        rc = -ENS_INTERR;
        goto end;
    }

    // set memory to 0, so not-deleted flag is 0
    memset(data, 0, fs->entry_size);
    if (flash_area_write(fs->area, offset, data, fs->entry_size)) {
        // writing was not successful
        rc = -ENS_INTERR;
    }

end:
    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_page_erase(ens_fs_t* fs, uint64_t entry_id, uint64_t sector_count) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // calculate the next page start before (or at) the given entry_id
    uint64_t start = (entry_id - entry_id % fs->sector_size) * fs->entry_size;

    // erase given amount of pages, starting for the given offset
    if (flash_area_erase(fs->area, start, fs->sector_size * sector_count)) {
        rc = -ENS_INTERR;
    }

    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}
