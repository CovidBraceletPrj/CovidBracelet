/*
 * Copyright (c) 2021 Louis Meyer and Patrick Rathje
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENS_RECORDS_H
#define ENS_RECORDS_H

#include "sequencenumber.h"
#include "storage.h"

typedef struct record_iterator {
    record_t current;
    record_sequence_number_t sn_next;
    record_sequence_number_t sn_end;  // the last sn to include
    uint8_t finished;
} record_iterator_t;

// Also uses start and end from storage if NULL pointers given
// iterate over a sequence number range of records (Null will select the latest and newest for start / end)
// automatically
int ens_records_iterator_init_range(record_iterator_t* iterator,
                                    record_sequence_number_t* opt_start,
                                    record_sequence_number_t* opt_end);

// TODO: This function should call with the relevant start and end sequence numbers (retrieved through e.g. binary
// search / metadata)
// TODO: Do we guarantee that higher sequence numbers have at least our timestamp and lower sequence numbers up to our
// timestamp?
int ens_records_iterator_init_timerange(record_iterator_t* iterator, uint32_t* ts_start, uint32_t* ts_end);

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
// load_contact function is thread safe?!)
uint8_t ens_records_iterate_with_callback(record_iterator_t* iter, ens_record_iterator_cb_t cb, void* userdata);

#endif