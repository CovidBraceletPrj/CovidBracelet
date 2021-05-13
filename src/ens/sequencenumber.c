#include "sequencenumber.h"

#define SN_MASK 0xffffff

/**
 * Mask a given sequence number to get rid of MSB.
 */
#define GET_MASKED_SN(x) (x & SN_MASK)

int sn_equal(record_sequence_number_t a, record_sequence_number_t b) {
    return GET_MASKED_SN(a) == GET_MASKED_SN(b);
}

record_sequence_number_t sn_increment(record_sequence_number_t sn) {
    return GET_MASKED_SN(++sn);
}

record_sequence_number_t sn_increment_by(record_sequence_number_t sn, uint32_t amount) {
    return GET_MASKED_SN((sn + amount));
}

record_sequence_number_t sn_get_middle_sn(record_sequence_number_t older, record_sequence_number_t newer) {
    if (older < newer) {
        return (older + newer) / 2;
    }

    // TODO lome: cover case for older > newer
}