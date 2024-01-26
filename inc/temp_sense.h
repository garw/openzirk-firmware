#include <zephyr/kernel.h>

enum TempSensorStatus {
    COLD,
    WARM,
    INVALID,
};

struct TempSensors {
    atomic_t vl_status;
    atomic_t rl_status;
};

void temp_sense_init();
void temp_sense_main(void *p1, void* p2, void *p3);
