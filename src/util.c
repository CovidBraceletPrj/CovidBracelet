#include "time.h"
#include <zephyr.h>
// TODO: this is very basic atm
uint32_t time_get_unix_seconds() {
    return k_uptime_get() / 1000;
}