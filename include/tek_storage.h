/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef tek_storage_H
#define tek_storage_H

#include <exposure-notification.h>
#include <zephyr/types.h>
#include "utility/sequencenumber.h"
#include "record_storage.h"

typedef struct tek {
    uint32_t timestamp;     // Seconds from january first 2000 (UTC+0)
    ENPeriodKey tek;        // the temporary exposure key
} __packed tek_t;

/**
 * Initializes the contact storage component
 * @param clean flag for indicating, if storage shall be init with clean state
 *
 * @return 0 for success
 */
int tek_storage_init(bool clean);

int tek_storage_add(tek_t* src);
int tek_storage_delete(tek_t* src);

/**
 * Returns 0 on success, else non zero (if none available)
 */
int tek_storage_get_latest_at_ts(tek_t* dest, uint32_t timestamp);

#endif