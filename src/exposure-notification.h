/*
 * Copyright (c) 2020 Patrick Rathje
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EXPOSURE_NOTIFICATION_H
#define EXPOSURE_NOTIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif


#include <time.h>
#include <stdint.h>
#include <stddef.h>

#ifndef EN_TEK_ROLLING_PERIOD
#define EN_TEK_ROLLING_PERIOD 144
#endif


#ifndef EN_INTERVAL_LENGTH
#define EN_INTERVAL_LENGTH 600
#endif

#ifndef EN_INIT_MBEDTLS_ENTROPY
#define EN_INIT_MBEDTLS_ENTROPY 1
#endif

/*
 * Type:  _ENBaseKey
 * --------------------
 * 16 Byte base key definition
 */
typedef struct _ENBaseKey { unsigned char b[16];} _ENBaseKey;


/*
 * Type:  ENIntervalNumber
 * --------------------
 * The number of the 10 minute interval
 */
typedef uint32_t ENIntervalNumber;


/*
 * Type:  ENPeriodKey
 * --------------------
 * 16 Byte temporary exposure key used throughout a rolling period (one day)
 */
typedef struct _ENBaseKey ENPeriodKey;


/*
 * Type:  ENPeriodIdentifierKey
 * --------------------
 * 16 Byte key used throughout each period to encrypt the identifier for each interval
 */
typedef struct _ENBaseKey ENPeriodIdentifierKey;


/*
 * Type:  ENIntervalIdentifier
 * --------------------
 * 16 Byte identifier used within the packet
 */
typedef struct _ENBaseKey ENIntervalIdentifier;


/*
 * Type:  ENPeriodMetadataEncryptionKey
 * --------------------
 * 16 Byte key used throughout each interval to encrypt the metadata within one period
 */
typedef struct _ENBaseKey ENPeriodMetadataEncryptionKey;


/*
 * Type:  ENPeriodMetadataEncryptionKey
 * --------------------
 * 16 Byte key used throughout each interval to encrypt the metadata within one period
 */
typedef struct _ENBaseKey ENPeriodMetadataEncryptionKey;


typedef int (*ENRandomBytesCallback)(void *buf, size_t len);

/*
 * Function:  en_init 
 * --------------------
 * Initializes the random number generator. Use NULL to initialize with the fallback random_bytes_func
 */
int en_init(ENRandomBytesCallback user_random_bytes_callback);

/*
 * Function:  en_get_interval_number 
 * --------------------
 * computes the interval number according to the given unix timestamp. Each interval lasts for EN_INTERVAL_LENGTH secs (defaults to 10 minutes).
 */
ENIntervalNumber en_get_interval_number(time_t timestamp);

/*
 * Function:  en_get_period_start_interval_number 
 * --------------------
 * computes the first interval number for the period given by the unix timestamp.
 */
ENIntervalNumber en_get_interval_number_at_period_start(time_t timestamp);


/*
 * Function:  en_generate_period_key 
 * --------------------
 * Generates a new period key using a cryptographic number generator. Used when the next period is dues (once a day).
 */
void en_generate_period_key(ENPeriodKey* periodKey);

/*
 * Function:  en_derive_period_identifier_key 
 * --------------------
 * Derives the interval identifier key based on the current period key
 */
void en_derive_period_identifier_key(ENPeriodIdentifierKey *periodIdentifierKey, const ENPeriodKey *periodKey);


/*
 * Function:  en_derive_interval_identifier 
 * --------------------
 * Get the identifier for a specific interval based on the identifierKey
 */
void en_derive_interval_identifier(ENIntervalIdentifier *intervalIdentifier, const ENPeriodIdentifierKey *periodIdentifierKey, const ENIntervalNumber intervalNumber);

/*
 * Function:  en_derive_period_metadata_encryption_key 
 * --------------------
 * Derives the interval metadata encryption key based on the current period key
 */
void en_derive_period_metadata_encryption_key(ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENPeriodKey *periodKey);


/*
 * Function:  en_encrypt_interval_metadata
 * --------------------
 * Encrypts the metadata.
 */
void en_encrypt_interval_metadata(const ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENIntervalIdentifier *intervalIdentifier, unsigned char *input, unsigned char *output, size_t inputSize);

/*
 * Function:  en_decrypt_interval_metadata
 * --------------------
 * Decrypts the metadata.
 */
void en_decrypt_interval_metadata(const ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey, const ENIntervalIdentifier *intervalIdentifier, unsigned char *input, unsigned char *output, size_t inputSize);

/*
 * Function:  en_generate_period_relevant_keys()
 * --------------------
 * This function generates all period keys at once (convenient right?)
 */
static inline void en_generate_and_derive_period_keys(ENPeriodKey *periodKey, ENPeriodIdentifierKey *periodIdentifierKey, ENPeriodMetadataEncryptionKey *periodMetadataEncryptionKey) {
    en_generate_period_key(periodKey);
    en_derive_period_identifier_key(periodIdentifierKey, periodKey);
    en_derive_period_metadata_encryption_key(periodMetadataEncryptionKey, periodKey);
}

#ifdef __cplusplus
}
#endif
#endif