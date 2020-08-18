/*
 * Copyright (c) 2020 Patrick Rathje
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#if EN_INCLUDE_ZEPHYR_DEPS
#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#endif

#include <string.h>

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"

#include "exposure-notification.h"



static mbedtls_ctr_drbg_context ctr_drbg;
static ENRandomBytesCallback random_bytes_callback = NULL;


int en_mbedtls_random_bytes_fallback(void *buf, size_t len) {
    return mbedtls_ctr_drbg_random(&ctr_drbg, buf, len);
}

#if EN_INIT_MBEDTLS_ENTROPY
int init_mbedtls_entropy() {
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init( &entropy );

    static char *personalization = "exposure-notification";

    mbedtls_ctr_drbg_init( &ctr_drbg );

    int ret = mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy,
                 (const unsigned char *) personalization,
                 strlen( personalization ) );
    return ret;
}
#endif
int en_init(ENRandomBytesCallback user_random_bytes_callback) {
    int ret = 0;


#if EN_INIT_MBEDTLS_ENTROPY
    ret = init_mbedtls_entropy();
#endif
    // Fallback to mbedtls random bytes implementation and init the entropy
    if (user_random_bytes_callback == NULL) {
        random_bytes_callback = en_mbedtls_random_bytes_fallback;
    } else {
        random_bytes_callback = user_random_bytes_callback;
    }

    return ret;
}

ENIntervalNumber en_get_interval_number(time_t timestamp) {
    return timestamp / EN_INTERVAL_LENGTH;
}

ENIntervalNumber en_get_interval_number_at_period_start(time_t timestamp) {
    ENIntervalNumber current = en_get_interval_number(timestamp);
    // round to period start
    return (current/EN_TEK_ROLLING_PERIOD)*EN_TEK_ROLLING_PERIOD;
}

void en_generate_period_key(ENPeriodKey* periodKey) {
    // TODO: error handling
    random_bytes_callback(periodKey->b, sizeof(periodKey->b));
}

void en_derive_period_identifier_key(ENPeriodIdentifierKey *periodIdentifierKey, const ENPeriodKey *periodKey) {

    const mbedtls_md_info_t *sha256_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    // TODO: Include \0 in the string?
    // TODO: Check return value
    // TODO: Is this utf-8 info correct?
    // TODO: Correct erros in en_get_period_metadata_encryption_key as well!
    const unsigned char info[7] = "EN-RPIK";
    mbedtls_hkdf(sha256_info, 0, 0, periodKey->b, sizeof(periodKey->b), info, sizeof(info), periodIdentifierKey->b, sizeof(periodIdentifierKey->b));
}

void en_derive_interval_identifier(ENIntervalIdentifier *intervalIdentifier, const ENPeriodIdentifierKey *periodIdentifierKey, const ENIntervalNumber intervalNumber) {
    unsigned char paddedData[16] = "EN-RPI";

    // copy intervalNumber in little endian format
    paddedData[12] = (intervalNumber >> 0) &0xFF;
    paddedData[13] = (intervalNumber >> 8) &0xFF;
    paddedData[14] = (intervalNumber >> 16) &0xFF;
    paddedData[15] = (intervalNumber >> 24) &0xFF;

    mbedtls_aes_context aes;
    mbedtls_aes_init( &aes );

    // Encrypt the padded data to get the identifier
    mbedtls_aes_setkey_enc( &aes, (const unsigned char*) periodIdentifierKey->b, sizeof(periodIdentifierKey->b) * 8 );
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, paddedData, intervalIdentifier->b);
    mbedtls_aes_free( &aes );
}

void en_derive_period_metadata_encryption_key(ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENPeriodKey *periodKey) {
    const mbedtls_md_info_t *sha256_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    const unsigned char info[7] = "EN-AEMK";
    mbedtls_hkdf(sha256_info, 0, 0, periodKey->b, sizeof(periodKey->b), info, sizeof(info), periodMetadataEncryptionKey->b, sizeof(periodMetadataEncryptionKey->b));
}


void en_encrypt_interval_metadata(const ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENIntervalIdentifier *intervalIdentifier, unsigned char *input, unsigned char *output, size_t inputSize) {
   
    mbedtls_aes_context aes;
    mbedtls_aes_init( &aes );

    // Encrypt the padded data to get the identifier
    size_t nonceOffset = 0;
    unsigned char nonceCounter[16]; 
    unsigned char streamBlock[16];

    // init nonce/IV to intervalIdentifer
    memcpy(nonceCounter, intervalIdentifier->b, sizeof(nonceCounter)); 
    memset(streamBlock, 0, sizeof(streamBlock)); 
    
    mbedtls_aes_setkey_enc( &aes, (const unsigned char*) periodMetadataEncryptionKey->b, sizeof(periodMetadataEncryptionKey->b) * 8 );
    mbedtls_aes_crypt_ctr(&aes, inputSize, &nonceOffset, nonceCounter, streamBlock, input, output);
    mbedtls_aes_free( &aes );
}

void en_decrypt_interval_metadata(const ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENIntervalIdentifier *intervalIdentifier, unsigned char *input, unsigned char *output, size_t inputSize) {  
   en_encrypt_interval_metadata(periodMetadataEncryptionKey, intervalIdentifier, input, output, inputSize);
}