/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COVID_H
#define COVID_H

#include "exposure-notification.h"

typedef struct period{
	ENPeriodKey periodKey;
	ENIntervalNumber periodInterval;
} __packed period_t;

int init_covid();
int do_covid();

bool get_infection();
void set_infection(bool _infected);
unsigned int get_period_cnt_if_infected();
period_t* get_period_if_infected(unsigned int id, size_t* size);

int get_index_by_interval(ENIntervalNumber periodInterval);


void print_periods();
#endif