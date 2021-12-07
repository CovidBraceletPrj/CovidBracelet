#include <stdlib.h>
#include <string.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <protobuf-c.h>

#include "export.pb-c.h"
#include "extract_keys.h"


#define KEY_SIZE 16
#ifndef PROTOBUF_BLOCK_SIZE
#define PROTOBUF_BLOCK_SIZE 0
#endif
#define EXPORT_BUFFER_SIZE 100000

void process_key(TemporaryExposureKey* key) {
    printk("Attempting to process key\n");
    // #if !COVID_MEASURE_PERFORMANCE
    if (key) {
        if (key->has_key_data) {
            int len = (key->key_data.len) + 1;
            char data[len + 1];
            memcpy(data, key->key_data.data, len - 1);
            data[len] = 0;
            printk("New key: %s\n", data);
        }
    }
    // #endif
}

size_t generate_keys(uint8_t** buffer_pointer, size_t max_size, int num_keys) {
    TemporaryExposureKeyExport export = TEMPORARY_EXPOSURE_KEY_EXPORT__INIT;
    // temporary_exposure_key_export__init(&export);
    TemporaryExposureKey** key_ptrs = (TemporaryExposureKey**) k_malloc(sizeof(TemporaryExposureKey*) * num_keys);
    if (key_ptrs == NULL) {
        printk("Could not allocate memory for pointers\n");
        return 0;
    }
    export.batch_num = 1;
    export.batch_size = 1;
    // TemporaryExposureKey* key_ptrs[num_keys];
    uint8_t key_data[KEY_SIZE];
    for (int i = 0; i < KEY_SIZE; i++) {
        key_data[i] = 0xFF;
    }
    TemporaryExposureKey key = TEMPORARY_EXPOSURE_KEY__INIT;
    // temporary_exposure_key__init(&key);
    key.key_data.data = key_data;
    key.key_data.len = KEY_SIZE;
    key.has_key_data = true;
    for (int i = 0; i < num_keys; i++) {
        key_ptrs[i] = &key;
    }
    export.n_keys = num_keys;
    export.keys = key_ptrs;
    size_t required_buffer = temporary_exposure_key_export__get_packed_size(&export);
    uint8_t* buffer = (uint8_t*) k_malloc(required_buffer);
    if (buffer == NULL) {
        printk("Could not allocate memory for buffer\n");
        return 0;
    }
    *buffer_pointer = buffer;
    if (buffer != NULL) {
        size_t buf_size = temporary_exposure_key_export__pack(&export, buffer);
        k_free(key_ptrs);
        return buf_size;
    } else {
        printk("Buffer too small to serialize %d keys. %d bytes necessary\n", num_keys, required_buffer);
        return 0;
    }
}

int unpack_keys(uint8_t* buf, size_t buf_size) {
    // Unpack protocol buffer
    printk("Buf size: %d\n", buf_size);
    TemporaryExposureKeyExport* export = temporary_exposure_key_export__unpack(NULL, buf_size, buf);
    if (export == NULL) {
        printk("error unpacking incoming message\n");
        return -2;
    }
    printk("Buffer %s\n", buf);
    printk("Batch num: %d, batch size: %d\n", export->batch_num, export->batch_size);
    printk("Number of keys: %d\n", export->n_keys);
    // Iterate over new keys
    for (int i = 0; i < export->n_keys; i++) {
        TemporaryExposureKey* key = export->keys[i];
        process_key(key);
    }

    // Iterate over revised keys
    for (int i = 0; i < export->n_revised_keys; i++) {
        TemporaryExposureKey* key = export->revised_keys[i];
        process_key(key);
    }
    return 0;
}

int test_unpacking(int num_keys) {
    uint8_t* buffer;
    printk("Test unpacking %d keys\n", num_keys);
    size_t buffer_size = generate_keys(&buffer, EXPORT_BUFFER_SIZE, num_keys);
    if (buffer_size) {
        unpack_keys(buffer, buffer_size);
    }
    k_free(buffer);
    return 0;
}