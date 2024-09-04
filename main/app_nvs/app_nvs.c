//
// Created by kok on 04.09.24.
//

#include "nvs_flash.h"

#include "esp_log.h"
#include "app_nvs.h"

static const char TAG[] = "app_nvs";

void app_nvs_init() {
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS successfully initialized");
}
