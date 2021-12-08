#ifndef EXTRACT_KEYS_H
#define EXTRACT_KEYS_H

#include "export.pb-c.h"

/**
 * @brief Process a key. This function could trigger the comparision between the key and those registered by the ENS.
 *
 * @param key A pointer to the Exposure key data structure
 */
void process_key(TemporaryExposureKey* key);

/**
 * @brief Generates a protocol buffer containing dummy keys.
 *
 * @param buffer_pointer A pointer to the pointer which will be used to reference the buffer externally. This will be
 * set to the memory area allocated to store the protocol buffer.
 * @param num_keys The number of keys that will be generated
 * @return size_t The size of the protocol buffer
 */
size_t generate_keys(uint8_t** buffer_pointer, int num_keys);

/**
 * @brief Unpacks the protocol buffer and iterates the `process_key` function over all keys.
 *
 * @param buf a pointer to the buffer
 * @param buf_size the size of the buffer in bytes
 * @return int
 */
int unpack_keys(uint8_t* buf, size_t buf_size);

/**
 * @brief Generates an protocol buffer with a specified number of keys and measures the time to execute `unpack_keys`,
 * which unpacks the protocol buffer and iterates `process_key` over the keys.
 *
 * @param num_keys the number of keys that will be created
 * @return int
 */
int test_unpacking(int num_keys);

#endif