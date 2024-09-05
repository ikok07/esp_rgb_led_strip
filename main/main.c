#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "portmacro.h"

#include "app_nvs/app_nvs.h"
#include "wifi_app/wifi_app.h"
#include "http_server/http_server.h"
#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"

void app_main() {

    // Initialize NVS
    app_nvs_init();

    wifi_app_init();
    http_server_init();
    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
