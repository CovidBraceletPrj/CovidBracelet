#include "sequencenumber.h"

record_sequence_number_t sn_mask(record_sequence_number_t sn) {
    return (sn & SN_MASK);
}

int sequence_number_eq(record_sequence_number_t a, record_sequence_number_t b) {
    return sn_mask(a) == sn_mask(b);
}

record_sequence_number_t sequence_number_increment(record_sequence_number_t sn) {
    return sn_mask(++sn);
}