#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "portmacro.h"

#include "wifi_app/wifi_app.h"
#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"

void app_main() {

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_app_init();
    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
