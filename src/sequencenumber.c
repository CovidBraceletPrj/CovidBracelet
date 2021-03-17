#include "sequencenumber.h"


int sequence_number_eq(record_sequence_number_t a, record_sequence_number_t b) {
    return (a & SN_MASK) == (b & SN_MASK);
}

record_sequence_number_t sequence_number_increment(record_sequence_number_t sn) {
    return (++sn & SN_MASK);
}