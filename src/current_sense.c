#include "app.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/timing/timing.h>

static const struct adc_dt_spec adc_current_sense = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);


void current_sense_init() {
    if (!adc_is_ready_dt(&adc_current_sense)) {
        printk("Unable to init current sense ADC\n");
        return;
    }
    int err = adc_channel_setup_dt(&adc_current_sense);
    if (err < 0) {
        printk("Could not setup currense sense ADC channel (err=%d)\n", err);
        return;
    }
}

K_TIMER_DEFINE(adc_sample_timer, NULL, NULL);
#define SAMPLE_PERIOD_US 100
#define NET_FREQUENCY_PERIOD_MS 20
#define CURRENT_SENSE_ADC_SAMPLES (NET_FREQUENCY_PERIOD_MS*1000 / SAMPLE_PERIOD_US)
#define ROLLING_AVG_SIZE 6

/* Sample one period of the net frequency, filter with rolling average and return seen diff*/
int32_t get_current_sense_max_to_min_mv_diff() {
    int32_t mv_min = INT32_MAX;
    int32_t mv_max = INT32_MIN;
    int32_t rolling_avg_window[ROLLING_AVG_SIZE];
    int32_t rolling_sum = 0;
    size_t rolling_avg_current_index = 0;
    int64_t start_time = k_uptime_get();
    int64_t sample_start_time;
    int64_t one_sample_time;


    uint16_t adc_buf;
    struct adc_sequence sequence = {
        .buffer = &adc_buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(adc_buf),
    };
    (void)adc_sequence_init_dt(&adc_current_sense, &sequence);


    sample_start_time = k_uptime_get();

    timing_t adc_read_start;
    timing_t adc_read_stop;
    uint64_t adc_read_cycles = 0;
    timing_t wait_start;
    timing_t wait_stop;
    uint64_t wait_cycles = 0;
    timing_init();
    timing_start();

    k_timer_start(&adc_sample_timer, K_NO_WAIT, K_USEC(SAMPLE_PERIOD_US));
    for (size_t sample = 0; sample < CURRENT_SENSE_ADC_SAMPLES; ++sample) {
        wait_start = timing_counter_get();
        k_timer_status_sync(&adc_sample_timer);
        wait_stop = timing_counter_get();
        wait_cycles += timing_cycles_get(&wait_start, &wait_stop);
        adc_read_start = timing_counter_get();
        (void)adc_read_dt(&adc_current_sense, &sequence);
        int32_t val_mv = (int32_t)adc_buf;
        (void)adc_raw_to_millivolts_dt(&adc_current_sense, &val_mv);
        adc_read_stop = timing_counter_get();
        adc_read_cycles += timing_cycles_get(&adc_read_start, &adc_read_stop);

        if (sample >= ROLLING_AVG_SIZE) {
            rolling_sum -= rolling_avg_window[rolling_avg_current_index];
        }
        rolling_sum += val_mv;
        rolling_avg_window[rolling_avg_current_index] = val_mv;
        rolling_avg_current_index = (rolling_avg_current_index +1) % ROLLING_AVG_SIZE;

        if (sample >= ROLLING_AVG_SIZE) {
            int32_t rolling_avg = rolling_sum / ROLLING_AVG_SIZE;
            if (rolling_avg < mv_min) {
                mv_min = rolling_avg;
            } else if (rolling_avg > mv_max) {
                mv_max = rolling_avg;
            }
        }
    }
    uint64_t adc_timing_ns = timing_cycles_to_ns_avg(adc_read_cycles, CURRENT_SENSE_ADC_SAMPLES);
    uint64_t adc_timer_time = timing_cycles_to_ns_avg(wait_cycles, CURRENT_SENSE_ADC_SAMPLES);
    one_sample_time = k_uptime_get() - sample_start_time;

    timing_stop();
    k_timer_stop(&adc_sample_timer);
    if (enable_debug_output) {
        printk("Filtered sequence ended. min=%imV, max=%imV, Δ=%imV, t=%llims, oneS=%llims, avgAdcTime=%lluns, avgTimerWait=%lluns\n",
               mv_min, mv_max, mv_max-mv_min, k_uptime_get() - start_time, one_sample_time, adc_timing_ns, adc_timer_time);
    }
    return mv_max - mv_min;
}

bool is_pump_active() {
    const size_t required_active_sequences =
        DT_PROP(DT_PATH(zephyr_user), current_sensor_minimal_active_time_ms) / NET_FREQUENCY_PERIOD_MS;
    for (size_t i = 0; i < required_active_sequences; ++i) {
        int32_t mv_diff = get_current_sense_max_to_min_mv_diff();
        if (mv_diff < DT_PROP(DT_PATH(zephyr_user), current_sensor_threshold_mv)) {
            if (enable_debug_output) {
                printk("Sampling stopped after %i sequences. -> Inactive\n", i);
            }
            return false;
        }
    }
    if (enable_debug_output) {
        printk("Sampled %i sequences above threshold -> Active\n", required_active_sequences);
    }
    return true;
}


void current_sense_main(void *p1, void *p2, void *p3) {
    struct k_event *sensor_events = (struct k_event*)p1;

    current_sense_init();
    // wait for stabilization capacitor to be fully charged
    k_msleep(50);

    while(1) {
        if (is_pump_active()) {
            k_event_post(sensor_events, EVENT_CURRENT_SENSE);
        }
        // wait a bit before we start another sampling sequence
        k_msleep(100);
    }

}
