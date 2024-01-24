
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

static const struct adc_dt_spec adc_temp_rl = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);
static const struct adc_dt_spec adc_temp_vl = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 2);


void temp_sense_init() {
    if (!adc_is_ready_dt(&adc_temp_rl)) {
        printk("Unable to init temp RL ADC\n");
        return;
    }
    int err = adc_channel_setup_dt(&adc_temp_rl);
    if (err < 0) {
        printk("Could not setup temp RL ADC channel (err=%d)\n", err);
        return;
    }
    if (!adc_is_ready_dt(&adc_temp_vl)) {
        printk("Unable to init temp VL ADC\n");
        return;
    }
     err = adc_channel_setup_dt(&adc_temp_vl);
    if (err < 0) {
        printk("Could not setup temp VL ADC channel (err=%d)\n", err);
        return;
    }
    printk("Temp sense init complete\n");
}