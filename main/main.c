#include "freertos/FreeRTOS.h"
#include "portmacro.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rmt/rmt_app.h"

void app_main() {
    rmt_app_start();
}
