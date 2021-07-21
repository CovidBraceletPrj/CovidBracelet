#ifndef BLOOM_H
#define BLOOM_H

#include "ens/storage.h"

/**
 * Initialize the bloom filter on basis of the already registerred records.
 */
int bloom_init();

// TODO lome: maybe only use RPI (should be sufficient)
int bloom_add_record(record_t* record);

// TODO lome: maybe only use RPI (should be sufficient)
int bloom_probably_has_record(record_t* record);

#endif