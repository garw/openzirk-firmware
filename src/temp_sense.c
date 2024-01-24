#include "output.h"

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

struct temp_sensor_measurements {
    int32_t vl_mv;
    int32_t rl_mv;
};

void get_temp_probes_raw(struct temp_sensor_measurements *measurements) {
    set_temp_probes_enable(1);
    // wait for stabilization
    k_msleep(2);
    uint16_t adc_buf;

    struct adc_sequence sequence = {
        .buffer = &adc_buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(adc_buf),
    };
    (void)adc_sequence_init_dt(&adc_temp_rl, &sequence); 
    (void)adc_read_dt(&adc_temp_rl, &sequence);
    measurements->rl_mv = (int32_t)adc_buf;
    (void)adc_raw_to_millivolts_dt(&adc_temp_rl, &measurements->rl_mv);
    
    (void)adc_sequence_init_dt(&adc_temp_vl, &sequence); 
    (void)adc_read_dt(&adc_temp_vl, &sequence);
    measurements->vl_mv = (int32_t)adc_buf;
    (void)adc_raw_to_millivolts_dt(&adc_temp_vl, &measurements->vl_mv);

    set_temp_probes_enable(0);
}

int32_t temp_probe_mv_to_ohm(int32_t mv) {
    static const int32_t R_fixed = 1200;
    static const int32_t U_vv = 3270;
    return R_fixed*mv/(U_vv - mv); 
}

int32_t temp_probe_ohm_to_deg_C(int32_t ohm) {
    return (ohm - 1000)*100/385;
}

void do_temp_probe() {
    struct temp_sensor_measurements meas;
    get_temp_probes_raw(&meas);

    int32_t rl_ohm = temp_probe_mv_to_ohm(meas.rl_mv);
    int32_t vl_ohm = temp_probe_mv_to_ohm(meas.vl_mv);
    printk("Measures VL=%dmV, RL=%dmV, calculated VL=%dOhm, RL=%dOhm\n", meas.vl_mv, meas.rl_mv, vl_ohm, rl_ohm);
    printk("Estimated VL=%d°C, RL=%d°C\n", temp_probe_ohm_to_deg_C(vl_ohm), temp_probe_ohm_to_deg_C(rl_ohm));
}