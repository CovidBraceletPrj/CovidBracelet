#ifndef BLOOM_H
#define BLOOM_H

#include "ens/storage.h"
#include "exposure-notification.h"

typedef struct bloom_filter {
    uint8_t* data;
    size_t size;
} bloom_filter_t;

/**
 * Initialize the bloom filter on basis of the already registerred records.
 */
bloom_filter_t* bloom_init(size_t size);

void bloom_destroy(bloom_filter_t* bloom);

// TODO lome: maybe only use RPI (should be sufficient)
void bloom_add_record(bloom_filter_t* bloom, ENIntervalIdentifier* rpi);

// TODO lome: maybe only use RPI (should be sufficient)
bool bloom_probably_has_record(bloom_filter_t* bloom, ENIntervalIdentifier* rpi);

#endif
