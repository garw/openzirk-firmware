#include "app.h"
#include "io.h"
#include "temp_sense.h"

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
    static const int32_t U_vv = 3280;
    return R_fixed*mv/(U_vv - mv);
}

int32_t temp_probe_ohm_to_deg_C(int32_t ohm) {
    return (ohm - 1000)*100/385;
}



void temp_sense_main(void *p1, void* p2, void *p3) {
    struct k_event *sensor_events = (struct k_event*)p1;
    struct TempSensors *sensors =(struct TempSensors *)p2;
    temp_sense_init();
    set_vl_led_blue();
    set_rl_led_blue();

    enum TempSensorStatus vl_status = INVALID;
    enum TempSensorStatus rl_status = INVALID;

    while (1) {
        struct temp_sensor_measurements meas;
        get_temp_probes_raw(&meas);

        int32_t rl_ohm = temp_probe_mv_to_ohm(meas.rl_mv);
        int32_t vl_ohm = temp_probe_mv_to_ohm(meas.vl_mv);

        uint32_t temp_vl_deg_c = temp_probe_ohm_to_deg_C(vl_ohm);
        uint32_t temp_rl_deg_c = temp_probe_ohm_to_deg_C(rl_ohm);

        if (enable_debug_output) {
            printk("Measures VL=%dmV, RL=%dmV, calculated VL=%dOhm, RL=%dOhm\n", meas.vl_mv, meas.rl_mv, vl_ohm, rl_ohm);
            printk("Estimated VL=%d°C, RL=%d°C\n", temp_vl_deg_c, temp_rl_deg_c);
        }

        if (temp_vl_deg_c > 200) {
            vl_status = INVALID;
            atomic_set(&sensors->vl_status, INVALID);
            toggle_vl_leds();
        }
        else if (temp_vl_deg_c > DT_PROP(DT_PATH(zephyr_user), temp_warm_threshold)) {
            set_vl_led_orange();
            atomic_set(&sensors->vl_status, WARM);
            if (vl_status != WARM) {
                k_event_post(sensor_events, EVENT_VL_WARM);
            }
            vl_status = WARM;
        } else {
            set_vl_led_blue();
            atomic_set(&sensors->vl_status, COLD);
            if (vl_status != COLD) {
                k_event_post(sensor_events, EVENT_VL_COLD);
            }
            vl_status = COLD;
        }

        if (temp_rl_deg_c > 200) {
            rl_status = INVALID;
            atomic_set(&sensors->rl_status, INVALID);
            toggle_rl_leds();
        } else if (temp_rl_deg_c > DT_PROP(DT_PATH(zephyr_user), temp_warm_threshold)) {
            set_rl_led_orange();
            atomic_set(&sensors->rl_status, WARM);
            if (rl_status != WARM) {
                k_event_post(sensor_events, EVENT_RL_WARM);
            }
            rl_status = WARM;
        } else {
            set_rl_led_blue();
            atomic_set(&sensors->rl_status, COLD);
            if (rl_status != COLD) {
                k_event_post(sensor_events, EVENT_RL_COLD);
            }
            rl_status = COLD;
        }

        k_msleep(1000);
    }
}
