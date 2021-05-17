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
    if (get_num_records() == 0) {
        iterator->finished = true;  // no contacts -> no iteration :)
    } else {
        iterator->finished = false;
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
// TODO lome: maybe add flag for indicating whether older or newer contact shall be loaded
int find_record_via_binary_search(record_t* record,
                                  uint32_t target,
                                  record_sequence_number_t start,
                                  record_sequence_number_t end) {
    // TODO lome: 1. handle entries with deleted/invalid data -> load left/right entry
    // TODO lome: 2. handle possible deadlocks because of 1.
    // TODO lome: 3. check, if oldest ts is newer than the latest -> return error that needs to be handled by the
    //              calling function
    record_t start_record;
    record_t end_record;

    // load the initial start and end record
    int rc = load_record(&start_record, start);
    if (rc) {
        return rc;
    }
    rc = load_record(&end_record, end);
    if (rc) {
        return rc;
    }

    do {
        // TODO lome: first entry for start, last entry for end
        // calculate the contact in the middle between start and end and load it
        record_sequence_number_t middle = (start_record.sn + end_record.sn) / 2;
        int rc = load_record(record, middle);
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

// TODO: This iterator does neither check if the sequence numbers wrapped around while iteration. As a result, first
// results could have later timestamps than following entries
int ens_records_iterator_init_timerange(record_iterator_t* iterator, uint32_t* ts_start, uint32_t* ts_end) {
    record_sequence_number_t oldest_sn = get_oldest_sequence_number();
    record_sequence_number_t latest_sn = get_latest_sequence_number();

    // try to find the oldest contact in our timerange
    record_t start_rec;
    int rc = load_record(&start_rec, oldest_sn);
    if (rc) {
        return rc;
    }
    // if starting timestamp lies in our bounds, perform binary search
    // TODO lome: check, if ts_start and ts_end are NULL -> use oldest/latest sn then
    // TODO lome: move oldest_sn and latest_sn to binary_search function
    // TODO lome: maybe keep track of the oldest and newest timestamp (optimization for later)
    if (start_rec.timestamp < *ts_start) {
        // TODO lome: only "return" sn, not actual record
        rc = find_record_via_binary_search(&start_rec, *ts_start, oldest_sn, latest_sn);

        if (rc) {
            return rc;
        }
    }

    // try to find the newest contact within out timerange
    record_t end_rec;
    rc = load_record(&end_rec, latest_sn);
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
        int res = load_record(&contact, iter->sn_next);

        if (!res) {
            next = &iter->current;
            memcpy(next, &contact, sizeof(record_t));
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
    iter->sn_next = 0;
    iter->sn_end = 0;
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