#ifndef ENS_FS_H
#define ENS_FS_H

#include <device.h>
#include <fs/fs.h>
#include <kernel.h>
#include <stdint.h>
#include <storage/flash_map.h>

typedef struct ens_fs {
    /**
     * Flash area for this file system.
     */
    const struct flash_area* area;
    /**
     * Size of each individual entry. The last byte will be used by
     * ens_fs to store metadata about each individual entry.
     *
     * @attention has to be multiple of drivers write size
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
    struct k_mutex ens_fs_lock;
    /**
     * Size for entries, which is used interally.
     *
     * usually 2^(ceil(log(entry_size + 1)))
     */
    // TODO lome: maybe introduce macro for this?
    size_t interal_size;
    /**
     * Buffer of size internal_size for performing actions regarding fs entries. This is needed, because internal_size
     * differs from entry_size and we need a buffer for working with our interal datatype.
     */
    // TODO lome: maybe move this into functions where it's needed?
    uint8_t* buffer;
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
int ens_fs_init(ens_fs_t* fs, uint8_t flash_id, uint64_t entry_size);

/**
 * Read an entry from this file system.
 *
 * @param fs file system
 * @param id id of entry
 * @param data destination address of data to be read
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_read(ens_fs_t* fs, uint64_t id, void* dest);

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
 * @param id id of the entry to erase the page for
 * @param sector_count count of sectors to erase
 *
 * @return 0 on success, -errno otherwise
 */
int ens_fs_page_erase(ens_fs_t* fs, uint64_t id, uint64_t sector_count);

#endif