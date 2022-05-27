/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef RECORD_STORAGE_H
#define RECORD_STORAGE_H

#include <zephyr/types.h>
#include "utility/sequencenumber.h"
#include <exposure-notification.h>



/**
 * RECORD STORAGE
 */
typedef uint64_t storage_id_t;

typedef struct bt_metadata {
    uint8_t version;
    uint8_t tx_power;
    uint8_t rsv1;
    uint8_t rsv2;
} __packed bt_metadata_t;

typedef struct associated_encrypted_metadata {
    uint8_t data[sizeof(bt_metadata_t)];
} __packed associated_encrypted_metadata_t;

typedef struct record {
    record_sequence_number_t sn;  // TODO: Convert Sequence Number
    uint32_t timestamp;           // TODO: Seconds from january first 2000 (UTC+0)
    uint8_t rssi;                  // TODO: Check correct
    ENIntervalIdentifier rolling_proximity_identifier;
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
int record_storage_init(bool clean);

/**
 * Reset state of record storage.
 */
void reset_record_storage();

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

/**
 * RECORD ITERATOR
 */
typedef struct record_iterator {
    /**
     * @internal
     */
    record_t current;
    /**
     * @internal
     */
    record_sequence_number_t sn_next;
    /**
     * @internal
     */
    record_sequence_number_t sn_end;  // the last sn to include
    /**
     * @internal
     */
    uint8_t finished;
} record_iterator_t;

// Also uses start and end from storage if NULL pointers given
// iterate over a sequence number range of records (Null will select the latest and newest for start / end)
// automatically
int ens_records_iterator_init_range(record_iterator_t* iterator,
                                    record_sequence_number_t* opt_start,
                                    record_sequence_number_t* opt_end);

// TODO: Do we guarantee that higher sequence numbers have at least our timestamp and lower sequence numbers up to our
// timestamp?
int ens_records_iterator_init_timerange(record_iterator_t* iterator, time_t* ts_start, time_t* ts_end);

record_t* ens_records_iterator_next(record_iterator_t* iter);

int ens_record_iterator_clear(record_iterator_t* iter);

// TODO: Is a callback the easiest thing to do? -> now it is the iterator :)
// TODO: should we really indicated the last record by sending a NULL pointer?
typedef uint8_t (*ens_record_iterator_cb_t)(const record_t* record, void* userdata);

enum {
    ENS_RECORD_ITER_STOP,      // STOP the iteration, this would not result in a NULL callback after the end is reached
    ENS_RECORD_ITER_CONTINUE,  // CONTINUE the iteration, this results in a NULL callback after the end is reached
};

// TODO: this function could be made asynchronous to handle delays in contact_storage reads?!
// TODO: How can we handle iteration while records are being added or deleted? (should be safe as long as the
// load_record function is thread safe?!)
uint8_t ens_records_iterate_with_callback(record_iterator_t* iter, ens_record_iterator_cb_t cb, void* userdata);

#endif