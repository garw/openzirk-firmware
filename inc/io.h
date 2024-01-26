#include <stdio.h>

void io_init();

void set_vl_led_blue();
void set_vl_led_orange();
void toggle_vl_leds();

void set_rl_led_blue();
void set_rl_led_orange();
void toggle_rl_leds();

void set_debug_led_red(int state);
void set_debug_led_green(int state);

void set_temp_probes_enable(int state);

void set_pump(int state);

uint8_t get_dipswitch_status();