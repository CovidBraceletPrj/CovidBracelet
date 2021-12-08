#ifndef CONTACT_STORAGE_H
#define CONTACT_STORAGE_H

#include <zephyr/types.h>

#include "../covid_types.h"
#include "sequencenumber.h"

typedef uint64_t storage_id_t;

typedef struct record {
    record_sequence_number_t sn;  // TODO: Convert Sequence Number
    uint32_t timestamp;           // TODO: Seconds from january first 2000 (UTC+0)
    rssi_t rssi;                  // TODO: Check correct
    rolling_proximity_identifier_t rolling_proximity_identifier;
    associated_encrypted_metadata_t associated_encrypted_metadata;
} __packed record_t;

typedef struct stored_records_information {
    record_sequence_number_t oldest_contact;
    uint32_t count;
} stored_records_information_t;

/**
 * Initializes the contact storage component
 * @param clean flag for indicating, if storage shall be init with clean state
 *
 * @return 0 for success
 */
int init_record_storage(bool clean);

/**
 * Reset state of record storage.
 */
int reset_record_storage();

/**
 * Loads the record with number sn into the destination struct
 * @param dest
 * @param sn
 * @return 0 in case of success
 */
int load_record(record_t* dest, record_sequence_number_t sn);

/**
 * Stores the record from src with number sn, increases latest sequence number
 * @param sn
 * @param src
 * @return 0 in case of success
 */
int add_record(record_t* src);

/**
 * Deletes the record from storage with number sn
 * @param sn the sequence number to delete
 * @return 0 in case of success
 */
int delete_record(record_sequence_number_t sn);

/**
 * TODO: How to handle if none is available?
 * @return The latest available sequence number (Caution: can actually be lower than the oldes in case of a
 * wrap-around!)
 */
record_sequence_number_t get_latest_sequence_number();

/**
 * TODO: How to handle if none is available?
 * @return The oldest available sequence number
 */
record_sequence_number_t get_oldest_sequence_number();

/**
 * @return The amount of contacts, usually get_latest_sequence_number() - get_oldest_sequence_number()
 */
uint32_t get_num_records();

int get_sequence_number_interval(record_sequence_number_t* oldest, record_sequence_number_t* latest);

#endif
