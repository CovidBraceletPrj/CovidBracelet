#ifndef SEQUENCENUMBER_H
#define SEQUENCENUMBER_H

#include <stdint.h>

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
 * @return the incremented sequence number
 */
record_sequence_number_t sn_increment(record_sequence_number_t sn);

/**
 * Increment the given sequence number by a given amount.
 *
 * @param sn sequence number to increment
 * @return the incremented sequence number
 */
record_sequence_number_t sn_increment_by(record_sequence_number_t sn, uint32_t amount);

/**
 * Get the middle between to given sequence numbers, while handling a possible wrap-around.
 *
 * @param older sequence number which will be treated as the older one
 * @param newer sequence number which will be treated as the newer one
 * @return the sequence number in the middle
 */
record_sequence_number_t sn_get_middle_sn(record_sequence_number_t older, record_sequence_number_t newer);
#endif