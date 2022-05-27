#ifndef UTIL_H
#define UTIL_H

#include <exposure-notification.h>
#include "../record_storage.h" // TODO: this dependency is not great...

// TODO: We should use timeutil_sync

// return the current unix timestamp in seconds
uint32_t time_get_unix_seconds();

void print_key(_ENBaseKey* key);
void print_rpi(ENIntervalIdentifier* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

#endif  // UTIL_H