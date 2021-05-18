#include <string.h>

#include "../covid_types.h"
#include "ens_error.h"
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
 * @param record pointer to the location, where the found sn shall be stored
 * @param target timestamp for which to find the nearest entry for
 * @param greater flag for indicating, if the loaded sn shall correspond to a greater (1) or smaller (0) timestamp
 */
int find_sn_via_binary_search(record_sequence_number_t* sn_dest, uint32_t target, int greater) {
    record_sequence_number_t start = get_oldest_sequence_number();
    record_sequence_number_t end = get_latest_sequence_number();

    record_t dummyRec;

    do {
        // calculate the contact in the middle between start and end
        record_sequence_number_t middle = sn_get_middle_sn(start, end);

        // try to load it
        int rc = load_record(&dummyRec, middle);
        if (rc && rc != -ENS_DELENT && rc != -ENS_NOENT) {
            // if our error is not concerning invalid or deleted entries, we just want to return
            return rc;
        } else if (rc == -ENS_DELENT || rc == -ENS_NOENT) {
            int direction = 1;
            do {
                // increment the calculated "middle" by a certain amount, so we can try to load a new entry
                sn_increment_by(middle, direction);
                rc = load_record(&dummyRec, middle);

                // alternate around the previously calculated "middle"
                // this should avoid deadlocks, because we never read an entry twice
                direction += direction > 0 ? 1 : -1;
                direction *= -1;
            } while (middle >= start && middle <= end && (rc == -ENS_DELENT || rc == -ENS_NOENT));
        }

        // if we still have an error, just return it
        if (rc) {
            return rc;
        }

        // determine the new start and end
        if (dummyRec.timestamp > target) {
            start = dummyRec.sn;
        } else {
            end = dummyRec.sn;
        }

        // break, if we are at the exact timestamp or our start and end are next to each other
    } while (dummyRec.timestamp != target && (end - start) > 1);

    // TODO lome: maybe loop here aswell?
    // increment/decrement the found sn, depending on the greater flag
    record_sequence_number_t found = dummyRec.sn;
    if (dummyRec.timestamp > target && !greater) {
        found--;
    } else if (dummyRec.timestamp < target && greater) {
        found++;
    }

    *sn_dest = found;

    return 0;
}

// TODO: This iterator does neither check if the sequence numbers wrapped around while iteration. As a result, first
// results could have later timestamps than following entries
int ens_records_iterator_init_timerange(record_iterator_t* iterator, uint32_t* ts_start, uint32_t* ts_end) {
    record_sequence_number_t oldest_sn = 0;
    record_sequence_number_t newest_sn = 0;

    // assure that *ts_end > *ts_start
    if (ts_start && ts_end && *ts_end < *ts_start) {
        return 1;
    }

    if (ts_start) {
        int rc = find_sn_via_binary_search(&oldest_sn, *ts_start, 1);
        if (rc) {
            return rc;
        }
    } else {
        oldest_sn = get_oldest_sequence_number();
    }

    if (ts_end) {
        int rc = find_sn_via_binary_search(&newest_sn, *ts_end, 0);
        if (rc) {
            return rc;
        }
    } else {
        newest_sn = get_latest_sequence_number();
    }

    return ens_records_iterator_init_range(iterator, &oldest_sn, &newest_sn);
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