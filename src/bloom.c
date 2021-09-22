#include "bloom.h"
#include "ens/storage.h"
#include "exposure-notification.h"

bloom_filter_t* bloom_init(size_t size) {
    bloom_filter_t* bloom = k_calloc(1, sizeof(bloom_filter_t));
    if (!bloom) {
        return NULL;
    }
    bloom->size = size;
    bloom->data = k_calloc(size, sizeof(uint8_t));
    if (!bloom->data) {
        bloom->size = 0;
        k_free(bloom);
        return NULL;
    }
    return bloom;
}

void bloom_destroy(bloom_filter_t* bloom) {
    if (bloom) {
        k_free(bloom->data);
        k_free(bloom);
    }
}

void bloom_add_record(bloom_filter_t* bloom, ENIntervalIdentifier* rpi) {
    uint8_t* data = bloom->data;
    for (int i = 0; i < sizeof(*rpi); i += 2) {
        uint32_t hash = (rpi->b[i] << 8) | rpi->b[i + 1];
        hash %= bloom->size * 8;
        data[hash / 8] |= 1 << (hash % 8);
    }
}

bool bloom_probably_has_record(bloom_filter_t* bloom, ENIntervalIdentifier* rpi) {
    uint8_t* data = bloom->data;
    for (int i = 0; i < sizeof(*rpi); i += 2) {
        uint32_t hash = (rpi->b[i] << 8) | rpi->b[i + 1];
        hash %= bloom->size * 8;
        if (!(data[hash / 8] & (1 << (hash % 8)))) {
            return false;
        }
    }
    return true;
}
