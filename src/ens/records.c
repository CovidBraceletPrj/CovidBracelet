#include <string.h>

#include "../covid_types.h"
#include "records.h"
#include "sequencenumber.h"
#include "storage.h"

int ens_records_iterator_init_range(record_iterator_t* iterator,
                                    record_sequence_number_t* opt_start,
                                    record_sequence_number_t* opt_end) {
    iterator->sn_next = opt_start ? *opt_start : get_oldest_sequence_number();
    iterator->sn_end = opt_end ? *opt_end : get_latest_sequence_number();
    if (get_num_contacts() == 0) {
        iterator->finished = true;  // no contacts -> no iteration :)
    }
    return 0;
}

/**
 * Find an entry via binary search for the timestamp.
 *
 * @param record pointer to the location, where the loaded record shall be stored
 * @param target timestamp for which to find the nearest entry for
 * @param start lower bound for the binary search
 * @param end upper bound for the binary search
 */
int find_record_via_binary_search(record_t* record,
                                  uint32_t target,
                                  record_sequence_number_t start,
                                  record_sequence_number_t end) {
    record_t start_record;
    record_t end_record;

    // load the initial start and end record
    int rc = load_contact(&start_record, start);
    if (rc) {
        return rc;
    }
    rc = load_contact(&end_record, end);
    if (rc) {
        return rc;
    }

    do {
        // calculate the contact in the middle between start and end and load it
        record_sequence_number_t middle = (start_record.sn + end_record.sn) / 2;
        int rc = load_contact(record, middle);
        if (rc) {
            return rc;
        }

        // determine the new start and end
        if (record->timestamp > target) {
            memcpy(&start_record, record, sizeof(record_t));
        } else {
            memcpy(&end_record, record, sizeof(record_t));
        }

        // break, if we are at the exact timestamp or our start and end are next to each other
    } while (record->timestamp != target && (end_record.sn - start_record.sn) > 1);

    return 0;
}

int ens_records_iterator_init_timerange(record_iterator_t* iterator, uint32_t* ts_start, uint32_t* ts_end) {
    record_sequence_number_t oldest_sn = get_oldest_sequence_number();
    record_sequence_number_t latest_sn = get_latest_sequence_number();

    // try to find the oldest contact in our timerange
    record_t start_rec;
    int rc = load_contact(&start_rec, oldest_sn);
    if (rc) {
        return rc;
    }
    // if starting timestamp lies in our bounds, perform binary search
    if (start_rec.timestamp < *ts_start) {
        rc = find_record_via_binary_search(&start_rec, *ts_start, oldest_sn, latest_sn);

        if (rc) {
            return rc;
        }
    }

    // try to find the newest contact within out timerange
    record_t end_rec;
    rc = load_contact(&end_rec, latest_sn);
    if (rc) {
        return rc;
    }
    // if ending timestamp lies in our bounds, perform binary search
    if (end_rec.timestamp > *ts_end) {
        rc = find_record_via_binary_search(&end_rec, *ts_end, oldest_sn, latest_sn);

        if (rc) {
            return rc;
        }
    }

    ens_records_iterator_init_range(iterator, &start_rec.sn, &end_rec.sn);

    return 0;
}

record_t* ens_records_iterator_next(record_iterator_t* iter) {
    record_t* next = NULL;

    while (next == NULL && !iter->finished) {
        record_t contact;
        // try to load the next contact
        int res = load_contact(&contact, iter->sn_next);

        if (!res) {
            next = &iter->current;
            memcpy(&next->associated_encrypted_metadata, &contact.associated_encrypted_metadata,
                   sizeof(associated_encrypted_metadata_t));
            memcpy(&next->rolling_proximity_identifier, &contact.rolling_proximity_identifier,
                   sizeof(rolling_proximity_identifier_t));
            memcpy(&next->rssi, &contact.rssi, sizeof(rssi_t));
            memcpy(&next->sn, &iter->sn_next, sizeof(record_sequence_number_t));

            // TODO lome: timestamp?
        }

        if (sn_equal(iter->sn_next, iter->sn_end)) {
            iter->finished = true;  // this iterator will finish after this execution
        } else {
            // increase the current sn
            iter->sn_next = sn_increment(iter->sn_next);
        }
    }

    return next;
}

int ens_record_iterator_clear(record_iterator_t* iter) {
    // clear all relevant fields in the iterator
    iter->finished = true;
    memset(&iter->current, 0, sizeof(iter->current));
    return 0;
}

uint8_t ens_records_iterate_with_callback(record_iterator_t* iter, ens_record_iterator_cb_t cb, void* userdata) {
    record_t* cur = ens_records_iterator_next(iter);
    bool cont = true;

    while (cur != NULL && cont) {
        int cb_res = cb(cur, userdata);
        if (cb_res == ENS_RECORD_ITER_STOP) {
            cont = false;
        }
    }

    if (cont) {
        cb(NULL, userdata);  // we call the callback one last time but with null data
    }
    return 0;
}