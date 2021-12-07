#ifndef EXTRACT_KEYS_H
#define EXTRACT_KEYS_H

#include "export.pb-c.h"

void process_key(TemporaryExposureKey*);

size_t generate_keys(uint8_t**, size_t, int);

int unpack_keys(uint8_t*, size_t) ;

int test_unpacking(int);

#endif