//
// Created by kok on 01.09.24.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "object_sensor.h"
#include "tasks_common.h"
#include "rmt/rmt_app.h"

static const char TAG[] = "object_sensor";

SemaphoreHandle_t g_object_sensor_semaphore_handle;

/**
 * Handler executed from the object sensor's interrupt
 * @param arg additional handler arguments
 */
static void object_sensor_isr_handler(void *arg) {
    static uint32_t last_isr_time = 0;
    const uint32_t curr_time = xTaskGetTickCountFromISR();

    if ((curr_time - last_isr_time) > (200 / portTICK_PERIOD_MS)) {
        xSemaphoreGive(g_object_sensor_semaphore_handle);
        last_isr_time = curr_time;
    }
}

/**
 * Object sensor's task waiting for semaphore
 */
static void object_sensor_task(void *pvParams) {
    while (1) {
        if (xSemaphoreTake(g_object_sensor_semaphore_handle, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Object sensor event occurred");

            // Send message to the RMT Application
            rmt_app_send_message(RMT_APP_MSG_TOGGLE_LED);
        }
    }
}

void object_sensor_init(void) {
    ESP_LOGI(TAG, "Initializing object sensor's task");
    ESP_ERROR_CHECK(gpio_set_direction(OBJECT_SENSOR_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_intr_type(OBJECT_SENSOR_GPIO, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_set_pull_mode(OBJECT_SENSOR_GPIO, GPIO_PULLUP_ONLY));

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(OBJECT_SENSOR_GPIO, object_sensor_isr_handler, NULL));

    g_object_sensor_semaphore_handle = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        &object_sensor_task,
        "object_sensor_task",
        OBJECT_SENSOR_TASK_STACK_SIZE,
        NULL,
        OBJECT_SENSOR_TASK_PRIORITY,
        NULL,
        OBJECT_SENSOR_TASK_CORE_ID
    );
}
