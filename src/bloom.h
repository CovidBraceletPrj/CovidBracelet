#ifndef BLOOM_H
#define BLOOM_H

#include "ens/storage.h"

typedef uint32_t (*hash_function)(const void* data);

typedef struct bloom_hash {
    hash_function func;
    struct bloom_hash* next;
} bloom_hash_t;

typedef struct bloom_filter {
    bloom_hash_t* func;
    uint8_t* data;
    size_t size;
} bloom_filter_t;

/**
 * Initialize the bloom filter on basis of the already registerred records.
 */
bloom_filter_t* bloom_init(size_t size);

// TODO lome: maybe only use RPI (should be sufficient)
void bloom_add_record(bloom_filter_t* bloom, record_t* record);

// TODO lome: maybe only use RPI (should be sufficient)
bool bloom_probably_has_record(bloom_filter_t* bloom, record_t* record);

#endif