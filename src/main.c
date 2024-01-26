/*
 * Copyright (c) 2024 Garwin
 *
 */

#include "app.h"
#include "current_sense.h"
#include "temp_sense.h"
#include "io.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

/* 1000 msec = 1 sec */
#define MAINLOOP_MAX_EVENT_WAIT K_SECONDS(30)

#define DEFAULT_STACK_SIZE 512

K_THREAD_STACK_DEFINE(current_sense_thread_stack, DEFAULT_STACK_SIZE);
struct k_thread current_sense_thread;

K_THREAD_STACK_DEFINE(temp_sense_thread_stack, DEFAULT_STACK_SIZE);
struct k_thread temp_sense_thread;

K_EVENT_DEFINE(sensor_events);

bool enable_debug_output = false;

int main(void)
{
    printk("Application starting up.\n");
    io_init();
    uint8_t dipswitch_state = get_dipswitch_status();
    printk("Application config %x\n", dipswitch_state);
    if (dipswitch_state & 1) {
        printk("Debug output enabled\n");
        enable_debug_output = true;
    }

    struct TempSensors temp_sensors = {ATOMIC_INIT(INVALID), ATOMIC_INIT(INVALID)};


    (void)k_thread_create(&current_sense_thread, current_sense_thread_stack,
                          K_THREAD_STACK_SIZEOF(current_sense_thread_stack),
                          current_sense_main, &sensor_events, NULL, NULL, 2, K_USER,
                          K_NO_WAIT);
    (void)k_thread_create(&temp_sense_thread, temp_sense_thread_stack,
                          K_THREAD_STACK_SIZEOF(temp_sense_thread_stack),
                          temp_sense_main, &sensor_events, &temp_sensors, NULL, 2, K_USER,
                          K_NO_WAIT);

    int64_t last_pump_activation = k_uptime_get() -
                                   DT_PROP(DT_PATH(zephyr_user), pump_mandatory_cooldown_min) * 60 * 1000;

    while (1) {
        int32_t events = k_event_wait(&sensor_events, EVENT_CURRENT_SENSE, true, MAINLOOP_MAX_EVENT_WAIT);
        int64_t now = k_uptime_get();
        if (events == 0) {
            // still nothing we check when we last ran as we would like to activate the pump at least daily
            if (now - last_pump_activation < 24*60*60*1000) {
                // was run in the last 24h. Continue waiting
                continue;
            }
            printk("Issue daily cycle.\n");
        }
        // we run if RL temp isn't still "warm"
        if (atomic_get(&temp_sensors.rl_status) == WARM) {
            printk("Current sensor detected but RL still warm.");
            continue;
        }
        if (now - last_pump_activation < DT_PROP(DT_PATH(zephyr_user), pump_mandatory_cooldown_min)*60*1000) {
            printf("Current sensor detected but still in mandatory cooldown\n.");
            continue;
        }

        // now run for duty cycle time or until RL becomes warm again
        printk("Start pump.\n");
        set_pump(1);
        last_pump_activation = k_uptime_get();
        events = k_event_wait(&sensor_events, EVENT_RL_WARM, true,
                              K_SECONDS(DT_PROP(DT_PATH(zephyr_user), pump_duty_cycle_s)));
        if (events == 0) {
            printk("Cycle time ended.\n");
        } else {
            printk("RL sensor reports WARM\n");
        }
        printk("Stop pump\n");
        set_pump(0);
    }
    return 0;
}
