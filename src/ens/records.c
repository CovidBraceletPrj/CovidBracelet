#include <string.h>

#include "../covid_types.h"
#include "ens_error.h"
#include "records.h"
#include "sequencenumber.h"
#include "storage.h"

int ens_records_iterator_init_range(record_iterator_t* iterator,
                                    record_sequence_number_t* opt_start,
                                    record_sequence_number_t* opt_end) {

    // prevent any changes during initialization
    int rc = get_sequence_number_interval(&iterator->sn_next, &iterator->sn_end);
    if (rc == 0) {
        iterator->finished = false;

        // we override start and end with the optional values
        if (opt_start) {
            iterator->sn_next = *opt_start;
        }
        if (opt_end) {
            iterator->sn_end = *opt_end;
        }
    } else {
        iterator->finished = true;
    }

    return 0;
}




int64_t get_timestamp_for_sn(record_sequence_number_t sn) {
    record_t rec;
    if(load_record(&rec, sn) == 0) {
        return rec.timestamp;
    } else {
        return -1;
    }
}


enum record_timestamp_search_mode {
    RECORD_TIMESTAMP_SEARCH_MODE_MIN,
     RECORD_TIMESTAMP_SEARCH_MODE_MAX,
};

/**
 * Find an entry via binary search for the timestamp.
 *
 * @param record pointer to the location, where the found sn shall be stored
 * @param target timestamp for which to find the nearest entry for
 * @param greater flag for indicating, if the loaded sn shall correspond to a greater (1) or smaller (0) timestamp
 */
int find_sn_via_binary_search(record_sequence_number_t* sn_dest, uint32_t target, enum record_timestamp_search_mode search_mode) {

    record_sequence_number_t start_sn;
    record_sequence_number_t end_sn;

    // prevent any changes during binary search initialization

    int rc = get_sequence_number_interval(&start_sn, &end_sn);

    if (rc) {
        return rc;
    }

    record_sequence_number_t last_sn = start_sn; // used to check if ran into issues, e.g. could not load the entry or rounding errors

    while(!sn_equal(start_sn, end_sn)) {
        // calculate the sn in the middle between start and end
        record_sequence_number_t cur_sn = sn_get_middle_sn(start_sn, end_sn);
        
        if (sn_equal(cur_sn, last_sn)) {
            // if we already checked this entry -> we reduce our boundaries and try again
            // this also solves issues with rounding
            // TODO: This is not the best way...
            if (search_mode == RECORD_TIMESTAMP_SEARCH_MODE_MIN) {
                int64_t start_ts = get_timestamp_for_sn(start_sn);
                if (start_ts == -1 || start_ts < target) {
                    // we could not load this entry or this entry is strictly smaller than our target
                    start_sn = sn_increment(start_sn); // we can safely increment as start_sn < end_sn
                } else {
                    // we actually found the wanted entry!
                    end_sn = start_sn; // this will break our loop
                }
            } else {
                // we search for the biggest value among them
                int64_t end_ts = get_timestamp_for_sn(end_sn);
                if (end_ts == -1 || end_ts > target) {
                    // we could not load this entry or this entry is strictly bigger than our target
                    end_sn = sn_decrement(end_sn); // we can safely decrement as start_sn < end_sn
                } else {
                    // we actually found the wanted entry!
                    start_sn = end_sn; // this will break our loop
                }
            }
        } else {

            int64_t mid_ts = get_timestamp_for_sn(cur_sn);

            if (mid_ts >= 0) {
                if (target < mid_ts) {
                    end_sn = cur_sn;
                } else if (target > mid_ts) {
                    start_sn = cur_sn;
                } else {
                    // target == mid_ts
                    if (search_mode == RECORD_TIMESTAMP_SEARCH_MODE_MIN) {
                        // we search for the smallest value among them -> look before this item
                        end_sn = cur_sn;
                    } else {
                        // we search for the biggest value among them -> look after this item
                        start_sn = cur_sn;
                    }
                }
            } else {
                // some errors -> we keep the current sn and try to narrow our boundaries
            }            
        }
        last_sn = cur_sn;
    }

    *sn_dest = start_sn; // == end_sn

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
        int rc = find_sn_via_binary_search(&oldest_sn, *ts_start, RECORD_TIMESTAMP_SEARCH_MODE_MIN);
        if (rc) {
            return rc;
        }
    } else {
        oldest_sn = get_oldest_sequence_number();
    }

    if (ts_end) {
        int rc = find_sn_via_binary_search(&newest_sn, *ts_end, RECORD_TIMESTAMP_SEARCH_MODE_MAX);
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