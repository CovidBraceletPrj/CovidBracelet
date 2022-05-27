#include "time.h"
#include <zephyr.h>
#include "utility/util.h"


// TODO: this is very basic atm
uint32_t time_get_unix_seconds() {
    return k_uptime_get() / 1000;
}

void print_key(_ENBaseKey* key) {
    for (int i = 0; i < sizeof(key->b); i++) {
        printk("%02x", key->b[i]);
    }
}

void print_rpi(ENIntervalIdentifier* rpi) {
    for (int i = 0; i < sizeof(ENIntervalIdentifier); i++) {
        printk("%02x", rpi->b[i]);
    }
}

void print_aem(associated_encrypted_metadata_t* aem) {
    for (int i = 0; i < sizeof(associated_encrypted_metadata_t); i++) {
        printk("%02x", aem->data[i]);
    }
}