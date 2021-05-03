#include "sequencenumber.h"

int sn_equal(record_sequence_number_t a, record_sequence_number_t b) {
    return GET_MASKED_SN(a) == GET_MASKED_SN(b);
}

record_sequence_number_t sn_increment(record_sequence_number_t sn) {
    return GET_MASKED_SN(++sn);
}