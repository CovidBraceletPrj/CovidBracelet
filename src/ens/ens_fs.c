#include <drivers/flash.h>
#include <kernel.h>
#include <math.h>
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

    // check, if needed internal size fits onto one page
    uint64_t internal_size = pow(2, ceil(log(entry_size + 1) / log(2)));
    if (internal_size > info.size) {
        flash_area_close(fs->area);
        return -ENS_INVARG;
    }
    fs->entry_size = entry_size;
    fs->interal_size = internal_size;

    // allocate buffer and set it to 0
    void* ptr = k_malloc(internal_size);
    if (ptr == NULL) {
        flash_area_close(fs->area);
        return -ENS_INTERR;
    }
    memset(ptr, 0, internal_size);

    // init the lock for the fs
    k_mutex_init(&fs->ens_fs_lock);
    return 0;
}

int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dest) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // read the entry from flash
    uint64_t offset = id * fs->interal_size;
    if (flash_area_read(fs->area, offset, fs->buffer, fs->interal_size)) {
        // opening of flash area was not successful
        rc = -ENS_INTERR;
        goto end;
    }

    // store pointer to buffer in variable, so we don't have to write (fs->buffer) everytime
    uint8_t* obj = fs->buffer;

    // crc stored in entry
    uint8_t entryCRC = GET_CHECKSUM(obj[fs->entry_size]);

    // calculated crc
    uint8_t checkCRC = crc7_be(SEED, obj, fs->entry_size);

    // check, if the entry is corrupted
    int isInvalid = memcmp(&entryCRC, &checkCRC, 1);
    if (isInvalid) {
        // if checksum is not equal to calculated checksum or if deleted flag is not 1, set memory to 0
        memset(obj, 0, fs->interal_size);
        // we do not know, if object got deleted or data is corrupt
        rc = -ENS_NOENT;
        goto end;
    }

    // check, if checksum and not-deleted flag are as expected
    int isNotDeleted = obj[fs->entry_size] & 1;
    if (!isNotDeleted) {
        // entry got deleted
        rc = -ENS_DELENT;
        goto end;
    }

    memcpy(dest, obj, fs->entry_size);
end:
    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_write(ens_fs_t* fs, uint64_t id, void* data) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // copy data into interal buffer
    memcpy(fs->buffer, data, fs->entry_size);

    // set CRC and not-deleted-flag
    uint8_t* obj = fs->buffer;
    obj[fs->entry_size] = crc7_be(SEED, obj, fs->entry_size) | 1;

    uint64_t offset = id * fs->interal_size;
    if (flash_area_write(fs->area, offset, data, fs->interal_size)) {
        // writing to flash was not successful
        rc = -ENS_INTERR;
    }

    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_delete(ens_fs_t* fs, uint64_t id) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);
    uint64_t offset = id * fs->interal_size;

    // set memory to 0, so not-deleted flag is 0
    memset(fs->buffer, 0, fs->interal_size);
    if (flash_area_write(fs->area, offset, fs->buffer, fs->interal_size)) {
        // writing was not successful
        rc = -ENS_INTERR;
    }

    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}

int ens_fs_page_erase(ens_fs_t* fs, uint64_t entry_id, uint64_t sector_count) {
    int rc = 0;
    k_mutex_lock(&fs->ens_fs_lock, K_FOREVER);

    // calculate the next page start before (or at) the given entry_id
    uint64_t start = (entry_id - entry_id % fs->sector_size) * fs->interal_size;

    // erase given amount of pages, starting for the given offset
    if (flash_area_erase(fs->area, start, fs->sector_size * sector_count)) {
        rc = -ENS_INTERR;
    }

    k_mutex_unlock(&fs->ens_fs_lock);
    return rc;
}
