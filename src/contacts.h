/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTACTS_H
#define CONTACTS_H

#include <zephyr.h>
#include <zephyr/types.h>

#include "covid.h"
#include "covid_types.h"
#include "exposure-notification.h"

void print_key(_ENBaseKey* key);
void print_rpi(rolling_proximity_identifier_t* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

#endif