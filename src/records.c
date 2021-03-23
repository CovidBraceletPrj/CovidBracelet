#include <string.h>
#include "records.h"
#include "covid_types.h"


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

record_t* ens_records_iterator_next(record_iterator_t* iter) {
    if (iter->finished) {
        return NULL;
    }

    record_t* next = NULL;

    // What is this?
    while (next == NULL) {
        contact_t contact;
        // try to load the next contact
        int res = load_contact(&contact, iter->sn_next);

        if (!res) {
            next = &iter->current;
            memcpy(&next->associated_encrypted_metadata, &contact.associated_encrypted_metadata, sizeof(associated_encrypted_metadata_t));
            memcpy(&next->rolling_proximity_identifier, &contact.rolling_proximity_identifier, sizeof(rolling_proximity_identifier_t));
            memcpy(&next->rssi, &contact.max_rssi, sizeof(rssi_t));
            memcpy(&next->sn, &iter->sn_next, sizeof(record_sequence_number_t));

            // TODO lome: timestamp?
        }

        if (sequence_number_eq(iter->sn_next, iter->sn_end)) {
            iter->finished = true;  // this iterator will finish after this execution
        } else {
            // increase the current sn
            iter->sn_next = sequence_number_increment(iter->sn_next);
        }
    }

    return next;
}

int ens_record_iterator_clear(record_iterator_t* iter) {
    // TODO: memclear  iter->current ?
    // 
    iter->finished = true;
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