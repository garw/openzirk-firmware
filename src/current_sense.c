#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

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
#define CURRENT_SENSE_ADC_SAMPLES 40

int32_t get_current_sense_max_to_min_mv_diff() {
    int32_t mv_min = INT32_MAX;
    int32_t mv_max = INT32_MIN;
    uint16_t adc_buf;
    struct adc_sequence sequence = {
        .buffer = &adc_buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(adc_buf),
    };
    (void)adc_sequence_init_dt(&adc_current_sense, &sequence); 


    k_timer_start(&adc_sample_timer,K_NO_WAIT , K_USEC(500));
    for (size_t sample = 0; sample < CURRENT_SENSE_ADC_SAMPLES; ++sample) {
        k_timer_status_sync(&adc_sample_timer);
        (void)adc_read_dt(&adc_current_sense, &sequence);
        int32_t val_mv = (int32_t)adc_buf;
        (void)adc_raw_to_millivolts_dt(&adc_current_sense, &val_mv);
        if (val_mv < mv_min) {
            mv_min = val_mv;
        } else if (val_mv > mv_max) {
            mv_max = val_mv;
        }
    }
    k_timer_stop(&adc_sample_timer);
    printk("Collected samples min=%imV, max=%imV\n", mv_min, mv_max);
    return mv_max - mv_min;
}

void current_sense_main(void *p1, void *p2, void *p3) {
    current_sense_init();

    while(1) {
        int32_t mv_diff = get_current_sense_max_to_min_mv_diff();
        printk("Current sense delta %imV\n", mv_diff);
        k_msleep(500);
    }

}