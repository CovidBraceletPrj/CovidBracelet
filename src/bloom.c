#include "bloom.h"
#include "ens/storage.h"

bloom_filter_t* bloom_init(size_t size) {
    bloom_filter_t* bloom = k_calloc(1, sizeof(bloom_filter_t));
    bloom->size = size;
    bloom->data = k_malloc(size * sizeof(uint8_t));
    return bloom;
}

void bloom_delete(bloom_filter_t* bloom) {
    if (bloom) {
        while (bloom->func) {
            bloom_hash_t* h;
            h = bloom->func;
            bloom->func = h->next;
            k_free(h);
        }
        k_free(bloom->data);
        k_free(bloom);
    }
}

void bloom_add_record(bloom_filter_t* bloom, record_t* record) {
    bloom_hash_t* h = bloom->func;
    uint8_t* data = bloom->data;
    while (h) {
        uint32_t hash = h->func(record);
        hash %= bloom->size * 8;
        data[hash / 8] |= 1 << (hash % 8);
        h = h->next;
    }
}

bool bloom_probably_has_record(bloom_filter_t* bloom, record_t* record) {
    bloom_hash_t* h = bloom->func;
    uint8_t* data = bloom->data;
    while (h) {
        uint32_t hash = h->func(record);
        hash %= bloom->size * 8;
        if ((data[hash / 8] & (1 << (hash % 8))) == 0) {
            return false;
        }
        h = h->next;
    }
    return true;
}