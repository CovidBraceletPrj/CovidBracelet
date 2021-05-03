#ifndef SEQUENCENUMBER_H
#define SEQUENCENUMBER_H

#include <stdint.h>

// mask for sequence numbers (2^24 - 1)
#define SN_MASK 16777215

/**
 * Mask a given sequence number to get rid of MSB.
 */
#define GET_MASKED_SN(x) (x & SN_MASK)

typedef uint32_t record_sequence_number_t;

/**
 * Compare to sequence numbers for equality.
 *
 * @param a first sequence number
 * @param b second sequence number
 * @return 1, if sequence numbers are equal, 0 otherwise.
 */
int sn_equal(record_sequence_number_t a, record_sequence_number_t b);

/**
 * Increment the given sequence number. Wraps around, if 2^24 is reached.
 *
 * @param sn sequence number to increment
 * @return the incremented sequenced number
 */
record_sequence_number_t sn_increment(record_sequence_number_t sn);
#endif