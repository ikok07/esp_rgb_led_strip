//
// Created by kok on 01.09.24.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "object_sensor.h"
#include "tasks_common.h"

const char TAG[] = "object_sensor";

/**
 * Handler executed from the object sensor's interrupt
 * @param arg additional handler arguments
 */
static void object_sensor_isr_handler(void *arg) {

}

/**
 * Creates object sensor's task
 * @param pvParams additional task parameters
 */
static void object_sensor_task(void *pvParams) {

}

void object_sensor_init(void) {
    ESP_LOGI(TAG, "Initializing object sensor's task");
    gpio_set_direction(OBJECT_SENSOR_GPIO, GPIO_MODE_INPUT);
    gpio_set_intr_type(OBJECT_SENSOR_GPIO, GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(OBJECT_SENSOR_GPIO, GPIO_PULLUP_ONLY);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(OBJECT_SENSOR_GPIO, object_sensor_isr_handler, NULL);

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
