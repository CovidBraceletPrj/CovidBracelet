#include "sequencenumber.h"

record_sequence_number_t sn_mask(record_sequence_number_t sn) {
    return (sn & SN_MASK);
}

int sn_equal(record_sequence_number_t a, record_sequence_number_t b) {
    return sn_mask(a) == sn_mask(b);
}

record_sequence_number_t sn_increment(record_sequence_number_t sn) {
    return sn_mask(++sn);
}