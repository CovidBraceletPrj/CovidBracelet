#ifndef STORAGE_H
#define STORAGE_H

#include "contacts.h"

/**
 * Initialize the storage api.
 */
int init_storage(void);

/**
 * Store all contacts of a period in flash.
 */
int store_period_contacts(period_contacts_t contacts);

/**
 * Get all contacts for a certain period.
 */
period_contacts_t get_contacts_for_period(uint16_t period);

#endif