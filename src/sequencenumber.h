#ifndef SEQUENCENUMBER_H
#define SEQUENCENUMBER_H

#include <stdint.h>

// mask for sequence numbers (2^24 - 1)
#define SN_MASK 16777215

typedef uint32_t record_sequence_number_t;

/**
 * Compare to sequence numbers for equality.
 * 
 * @param a first sequence number
 * @param b second sequence number
 * @return 1, if sequence numbers are equal, 0 otherwise.
 */
int sequence_number_eq(record_sequence_number_t a, record_sequence_number_t b);

/**
 * Increment the given sequence number. Wraps around, if 2^24 is reached.
 *
 * @param sn sequence number to increment
 * @return the incremented sequenced number
 */
record_sequence_number_t sequence_number_increment(record_sequence_number_t sn);
#endif