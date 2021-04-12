#ifndef ENS_FS_H
#define ENS_FS_H

#include <device.h>
#include <kernel.h>
#include <storage/flash_map.h>

#include "fs/fs.h"
#include "stdint.h"

typedef struct ens_fs {
    /**
     * Flash area for this file system.
     */
    const struct flash_area* area;
    /**
     * Size of each individual entry.
     */
    size_t entry_size;
    /**
     * Size of each sector of this fs.
     */
    uint16_t sector_size;
    /**
     * Amount of sectors in this fs.
     */
    uint16_t sector_count;
    /**
     * Lock for this fs.
     */
    // struct k_mutex* lock;
} ens_fs_t;

/**
 * Initialize the file system.
 * 
 * @param fs file system
 * @param id id of the partition
 * @param size of each entry in the file-system. Has to power of 2
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_init(ens_fs_t* fs, uint8_t id, uint64_t entry_size);

/**
 * Read an entry from this file system.
 *
 * @param fs file system
 * @param id id of entry
 * @param data destination address of data to be read
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dist);

/**
 * Write data to the file system.
 *
 * @param fs file system
 * @param id id of the entry
 * @param data data to be written
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_write(ens_fs_t* fs, uint64_t id, void* data);

/**
 * Delete an entry from the file system.
 *
 * @param fs file system
 * @param id id of the entry to be deleted
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_delete(ens_fs_t* fs, uint64_t id);

/**
 * Erase a given amount of flash sectors. Starting at offset (if offset is not at page start, it will be rounded down).
 *
 * @param fs file system
 * @param offset offset to start erasing
 * @param sector_count count of sectors to erase
 * 
 * @return 0 on success, -errno otherwise
 */
int ens_fs_page_erase(ens_fs_t* fs, uint64_t offset, uint64_t sector_count);

#endif