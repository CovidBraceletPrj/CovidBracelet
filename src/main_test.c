/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>


void main(void) {
    while(1) {

        printk("Hello World!\n");
    }
}
