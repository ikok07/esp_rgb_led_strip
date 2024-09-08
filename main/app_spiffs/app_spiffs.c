//
// Created by kok on 08.09.24.
//

#include "esp_spiffs.h"
#include "esp_log.h"

#include "app_spiffs.h"

static const char TAG[] = "app_spiffs";

void app_spiffs_init(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    const esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 15,
        .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
    size_t total_spiffs_size, used_spiffs_size;
    esp_spiffs_info(NULL, &total_spiffs_size, &used_spiffs_size);
    ESP_LOGI(TAG, "SPIFFS successfully initialized");
    ESP_LOGI(TAG, "SPIFFS total size: %d, used size: %d", total_spiffs_size, used_spiffs_size);
}
