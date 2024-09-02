#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "portmacro.h"

#include "object_sensor/object_sensor.h"
// #include "rmt/rmt_app.h"

void app_main() {
    object_sensor_init();
    // rmt_app_start();
}
