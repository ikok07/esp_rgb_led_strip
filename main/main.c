#include <esp_spiffs.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "portmacro.h"
#include "esp_log.h"

#include "app_nvs/app_nvs.h"
#include "wifi_app/wifi_app.h"
#include "http_server/http_server.h"
#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"

void app_main() {

    // Initialize NVS
    app_nvs_init();

    // Initialize SPIFFS
    const esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 15,
        .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
    size_t total_spiffs_size, used_spiffs_size;
    esp_spiffs_info(NULL, &total_spiffs_size, &used_spiffs_size);
    ESP_LOGI("main", "SPIFFS total size: %d, used size: %d", total_spiffs_size, used_spiffs_size);

    wifi_app_init();
    http_server_init();
    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
