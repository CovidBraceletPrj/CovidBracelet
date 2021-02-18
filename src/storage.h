#ifndef CONTACT_STORAGE_H
#define CONTACT_STORAGE_H

#include "contacts.h"  // Requires contact_t in contacts.h!

// TODO: This should be masked to k=24 bits
typedef uint32_t record_sequence_number_t;

typedef uint16_t storage_id_t;

typedef struct stored_contacts_information {
    record_sequence_number_t oldest_contact;
    record_sequence_number_t latest_contact;
} stored_contacts_information_t;

/**
 * Initializes the contact storage component
 * @return 0 for success
 */
int init_contact_storage();

/**
 * Checks if a contact is available with number sn
 * @param sn
 * @return 1 if available, 0 otherwise or in case of an error
 */
int has_contact(record_sequence_number_t sn);

/**
 * Loads the contact with number sn into the destination struct
 * @param dest
 * @param sn
 * @return 0 in case of success
 */
int load_contact(contact_t* dest, record_sequence_number_t sn);

/**
 * Stores the contact from src with number sn, increases latest sequence number
 * @param sn
 * @param src
 * @return 0 in case of success
 */
int add_contact(contact_t* src);

/**
 * Deletes the contact from storage with number sn
 * @param sn the sequence number to delete
 * @return 0 in case of success
 */
int delete_contact(record_sequence_number_t sn);

/**
 * TODO: How to handle if none is available?
 * @return The latest available sequence number (Caution: can actually be lower than the oldes in case of a
 * wrap-around!)
 */
record_sequence_number_t get_latest_sequence_number();

/**
 * TODO: How to handle if none is available?
 * @return The oldest available sequence number
 */
record_sequence_number_t get_oldest_sequence_number();

/**
 * TODO: How to handle if none is available?
 * @return The amount of contacts, usually get_latest_sequence_number() - get_oldest_sequence_number()
 */
uint32_t get_num_contacts();

#endif