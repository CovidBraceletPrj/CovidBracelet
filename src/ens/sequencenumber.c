#include "sequencenumber.h"

#define SN_MASK 0xffffff

/**
 * Mask a given sequence number to get rid of MSB.
 */
#define GET_MASKED_SN(x) (x & SN_MASK)

bool sn_equal(record_sequence_number_t a, record_sequence_number_t b) {
    return GET_MASKED_SN(a) == GET_MASKED_SN(b);
}

record_sequence_number_t sn_increment(record_sequence_number_t sn) {
    return GET_MASKED_SN(++sn);
}

record_sequence_number_t sn_decrement(record_sequence_number_t sn) {
    if (sn > 0) {
        return GET_MASKED_SN((sn-1));
    } else {
        return SN_MASK;
    }
}

record_sequence_number_t sn_increment_by(record_sequence_number_t sn, uint32_t amount) {
    return GET_MASKED_SN((sn + amount));
}

record_sequence_number_t sn_decrement_by(record_sequence_number_t sn, uint32_t amount) {
    return sn_increment_by(sn, (SN_MASK+1)-amount);
}

record_sequence_number_t sn_get_middle_sn(record_sequence_number_t older, record_sequence_number_t newer) {
    if (older <= newer) {
        return GET_MASKED_SN(((older + newer) / 2));
    } else {
        return GET_MASKED_SN(((older + newer + SN_MASK) / 2));
    }
}
