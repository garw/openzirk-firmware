#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec pico_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec led_vl_b = GPIO_DT_SPEC_GET(DT_NODELABEL(led_vl_b), gpios);
static const struct gpio_dt_spec led_vl_o = GPIO_DT_SPEC_GET(DT_NODELABEL(led_vl_o), gpios);

static const struct gpio_dt_spec led_rl_b = GPIO_DT_SPEC_GET(DT_NODELABEL(led_rl_b), gpios);
static const struct gpio_dt_spec led_rl_o = GPIO_DT_SPEC_GET(DT_NODELABEL(led_rl_o), gpios);

static const struct gpio_dt_spec led_debug_r = GPIO_DT_SPEC_GET(DT_NODELABEL(led_debug_r), gpios);
static const struct gpio_dt_spec led_debug_g = GPIO_DT_SPEC_GET(DT_NODELABEL(led_debug_g), gpios);

static const struct gpio_dt_spec ssr = GPIO_DT_SPEC_GET(DT_NODELABEL(ssr), gpios);
static const struct gpio_dt_spec temp_probes_enable = GPIO_DT_SPEC_GET(DT_NODELABEL(temp_probes_enable), gpios);

static const struct gpio_dt_spec* all_gpios[] = {&pico_led, &led_vl_b, &led_vl_o, &led_rl_b, &led_rl_o,
                                                 &led_debug_r, &led_debug_g, &ssr, &temp_probes_enable};

void output_init() {
    for (size_t i = 0; i < ARRAY_SIZE(all_gpios); ++i) {
        int err = gpio_pin_configure_dt(all_gpios[i], GPIO_OUTPUT_INACTIVE);
        if (err < 0) {
            printk("Failed to init gpio index %d", i);
        }
    }
    gpio_pin_set_dt(&pico_led, 1);
}

void set_vl_led_blue() {
    (void)gpio_pin_set_dt(&led_vl_b, 1);
    (void)gpio_pin_set_dt(&led_vl_o, 0);
}
void set_vl_led_orange() {
    (void)gpio_pin_set_dt(&led_vl_o, 1);
    (void)gpio_pin_set_dt(&led_vl_b, 0);
}

void set_rl_led_blue() {
    (void)gpio_pin_set_dt(&led_rl_b, 1);
    (void)gpio_pin_set_dt(&led_rl_o, 0);
}
void set_rl_led_orange() {
    (void)gpio_pin_set_dt(&led_rl_o, 1);
    (void)gpio_pin_set_dt(&led_rl_b, 0);
}

void set_debug_led_red(int state) {
    (void)gpio_pin_set_dt(&led_debug_r, state);
}
void set_debug_led_green(int state) {
    (void)gpio_pin_set_dt(&led_debug_g, state);
}

void set_temp_probes_enable(int state) {
    (void)gpio_pin_set_dt(&temp_probes_enable, state);
}

void set_pump(int state) {
    (void)gpio_pin_set_dt(&ssr, state);
}