#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "portmacro.h"

#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"

void app_main() {
    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
