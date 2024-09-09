//
// Created by kok on 03.09.24.
//

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "tasks_common.h"
#include "rmt/rmt_app.h"
#include "mode_switcher.h"

#include "object_sensor/object_sensor.h"


static const char TAG[] = "mode_switcher";

SemaphoreHandle_t mode_switcher_semaphore_handle;


/**
 * Mode switcher ISR handler
 */
static void mode_switcher_isr_handler(void *arg) {
    static uint32_t last_isr_time = 0;
    const uint32_t curr_time = xTaskGetTickCountFromISR();

    if ((curr_time - last_isr_time) > (500 / portTICK_PERIOD_MS)) {
        xSemaphoreGive(mode_switcher_semaphore_handle);
    }
}

/**
 * Mode switcher's task
 */
static void mode_switcher_task(void *pvParams) {
    while (1) {
        if (xSemaphoreTake(mode_switcher_semaphore_handle, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Mode switcher event occurred");
            rmt_app_send_message(RMT_APP_MSG_CYCLE_MODE);
        }
    }
}

void mode_switcher_init(void) {
    ESP_LOGI(TAG, "Initializing mode_switcher's task");
    ESP_ERROR_CHECK(gpio_set_direction(MODE_SWITCHER_GPIO_NUM, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_intr_type(MODE_SWITCHER_GPIO_NUM, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_set_pull_mode(MODE_SWITCHER_GPIO_NUM, GPIO_PULLUP_ONLY));

    const esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    if (err != ESP_ERR_INVALID_STATE && err != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(MODE_SWITCHER_GPIO_NUM, mode_switcher_isr_handler, NULL));

    mode_switcher_semaphore_handle = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        &mode_switcher_task,
        "mode_switcher_task",
        MODE_SWITCHER_TASK_STACK_SIZE,
        NULL,
        MODE_SWITCHER_TASK_PRIORITY,
        NULL,
        MODE_SWITCHER_TASK_CORE_ID
    );
}
