/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "current_sense.h"
#include "temp_sense.h"
#include "output.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

#define DEFAULT_STACK_SIZE 512

K_THREAD_STACK_DEFINE(current_sense_thread_stack, DEFAULT_STACK_SIZE);
struct k_thread current_sense_thread;


int main(void)
{
    int ret;
    bool led_state = true;
    printk("Application starting up.\n");
    output_init();
    temp_sense_init();

    k_tid_t current_sense_thread_tid = k_thread_create(&current_sense_thread, current_sense_thread_stack,
                                                       K_THREAD_STACK_SIZEOF(current_sense_thread_stack),
                                                       current_sense_main, NULL, NULL, NULL, 2, K_USER,
                                                       K_NO_WAIT);


    while (1) {
        set_vl_led_blue();
        set_rl_led_orange();
        
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
