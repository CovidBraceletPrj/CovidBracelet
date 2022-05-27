#include <string.h>
#include "tek_storage.h"

int tek_storage_init(bool clean) {
    return 0;
}

int tek_storage_add(tek_t* src) {
    // TODO
    return -1;
}

int tek_storage_delete(tek_t* src) {
    // TODO
    return -1;
}

/**
 * Returns 0 on success, else non zero (if none available)
 * TODO: This is not really implemented...
 */
int tek_storage_get_latest_at_ts(tek_t* dest, uint32_t timestamp) {
    dest->timestamp = timestamp;
    memset(&dest->tek, 0, sizeof(dest->tek));
    return 0;
}