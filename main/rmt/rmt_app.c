//
// Created by kok on 31.08.24.
//

#include <string.h>

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"

#include "led_encoder/led_encoder.h"
#include "tasks_common.h"
#include "rmt_app.h"

/**
 * RMT Applicatiom messaging queue handle
 */
QueueHandle_t rmt_app_message_queue_handle = NULL;

static const char TAG[] = "rmt_app";

rmt_channel_handle_t g_tx_chan = NULL;
rmt_encoder_handle_t g_rmt_encoder = NULL;
rmt_transmit_config_t g_tx_config;
static rmt_app_mode_e g_rmt_app_sel_mode = RMT_APP_LED_OFF;

// --------- AUX RMT METHODS --------- //

/**
 * Convert HSV values to standard RGB for the LED strip
 */
static void led_strip_hsv2rgb(uint32_t hue, const uint32_t saturation, const uint32_t value, uint32_t *red, uint32_t *green, uint32_t *blue) {
    hue %= 360;
    const uint32_t rgb_max = value * 2.55f;
    const uint32_t rgb_min = rgb_max * (100 - saturation) / 100.0f;

    const uint32_t i = hue / 60;
    const uint32_t diff = hue % 60;

    // RGB adjustment amount by hue
    const uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
        case 0:
            *red = rgb_max;
            *green = rgb_min + rgb_adj;
            *blue = rgb_min;
        break;
        case 1:
            *red = rgb_max - rgb_adj;
            *green = rgb_max;
            *blue = rgb_min;
        break;
        case 2:
            *red = rgb_min;
            *green = rgb_max;
            *blue = rgb_min + rgb_adj;
        break;
        case 3:
            *red = rgb_min;
            *green = rgb_max - rgb_adj;
            *blue = rgb_max;
        break;
        case 4:
            *red = rgb_min + rgb_adj;
            *green = rgb_min;
            *blue = rgb_max;
        break;
        default:
            *red = rgb_max;
            *green = rgb_min;
            *blue = rgb_max - rgb_adj;
        break;
    }
}

// --------- RMT MESSAGE QUEUE --------- //

/**
 * RMT Application's message queue task
 */
static void rmt_app_msg_queue_task(void *pvParams) {
    rmt_app_message_t msg;
    while(1) {
        if (xQueueReceive(rmt_app_message_queue_handle, &msg, portMAX_DELAY)) {
            switch (msg.msgID) {
                case RMT_APP_MSG_TOGGLE_LED:
                    if (g_rmt_app_sel_mode) g_rmt_app_sel_mode = RMT_APP_LED_OFF;
                    else g_rmt_app_sel_mode = RMT_APP_LED_MODE1;
                    break;
            }
        }
    }
}

// --------- TRANSMIT RMT DATA --------- //

static rmt_app_transmit_config_t *rmt_app_new_transmit_config(
    const rmt_channel_handle_t tx_chan,
    const rmt_transmit_config_t tx_config,
    const rmt_encoder_handle_t rmt_encoder,
    const uint32_t *hue,
    const uint32_t *red,
    const uint32_t *green,
    const uint32_t *blue,
    const uint32_t *start_rgb
) {
    rmt_app_transmit_config_t *config = malloc(sizeof(rmt_app_transmit_config_t));
    config->tx_chan = tx_chan;
    config->tx_config = tx_config;
    config->rmt_encoder = rmt_encoder;
    if (hue != NULL) {
        config->hue = malloc(sizeof(uint32_t));
        if (config->hue == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for the hue variable!");
            abort();
        }
        *config->hue = *hue;
    }

    if (red != NULL) config->red = *red;
    if (green != NULL) config->green = *green;
    if (blue != NULL) config->blue = *blue;

    if (start_rgb != NULL) {
        config->start_rgb = malloc(sizeof(uint32_t));
        if (config->start_rgb == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for the start_rgb variable!");
            abort();
        }
        *config->start_rgb = *start_rgb;
    }

    return config;
}

/**
 * Transmit the rmt data to the LED
 * @param config RMT Application transmit configuration structure
 */
static void rmt_app_transmit_data(rmt_app_transmit_config_t *config) {
    for (int j = 0; j < RMT_APP_LED_NUMBERS; j += 3) {
        // Build RGB pixels
        if (config->hue != NULL) {
            *config->hue = j * 360 / RMT_APP_LED_NUMBERS;
            if (config->start_rgb != NULL) *config->hue += *config->start_rgb;
            led_strip_hsv2rgb(*config->hue, 100, 100, &config->red, &config->green, &config->blue);
        }
        config->led_strip_pixels[j * 3 + 0] = config->green;
        config->led_strip_pixels[j * 3 + 1] = config->blue;
        config->led_strip_pixels[j * 3 + 2] = config->red;
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(config->tx_chan, config->rmt_encoder, config->led_strip_pixels, sizeof(config->led_strip_pixels), &config->tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(config->tx_chan, portMAX_DELAY));
}

// --------- RMT LED MODE METHODS --------- //

/**
 * Turns the LED off
 */
static void rmt_app_led_off(void) {
    ESP_LOGI(TAG, "Selected RMT_APP_LED_OFF");
    rmt_app_transmit_config_t *config = rmt_app_new_transmit_config(g_tx_chan, g_tx_config, g_rmt_encoder, NULL, 0, 0, 0, NULL);
    rmt_app_transmit_data(config);
}

static void rmt_app_led_mode_1(void) {
    ESP_LOGI(TAG, "Selected RMT_APP_LED_MODE1");

    uint32_t hue = 0;
    static uint32_t start_rgb = 0;
    for (int i = 0; i < 3; i++) {
        rmt_app_transmit_config_t *config = rmt_app_new_transmit_config(
            g_tx_chan,
            g_tx_config,
            g_rmt_encoder,
            &hue, NULL, NULL, NULL, &start_rgb
        );
        rmt_app_transmit_data(config);
        vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
        memset(config->led_strip_pixels, 0, sizeof(config->led_strip_pixels));
        rmt_app_transmit_data(config);
        vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
        free(config->hue);
        free(config->start_rgb);
        free(config);
    }
    start_rgb += 60;
}

// --------- MAIN RMT METHODS --------- //

/**
 * RMT Application task
 */
static void rmt_app_task(void *pvParams) {
    while (1) {
        switch (g_rmt_app_sel_mode) {
            case RMT_APP_LED_OFF:
                rmt_app_led_off();
            case RMT_APP_LED_MODE1:
                rmt_app_led_mode_1();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void rmt_app_start(void) {
    // Configure and create RMT TX Channel
    const rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_APP_SRC_CLK,
        .gpio_num = RMT_APP_LED_GPIO_NUM,
        .resolution_hz = RMT_APP_RESOLUTION_HZ,
        .mem_block_symbols = RMT_APP_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = RMT_APP_TRANS_QUEUE_SIZE
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &g_tx_chan));

    // Configure and create the RMT Encoder
    const rmt_led_strip_encoder_config_t rmt_config = {
        .resolution = RMT_APP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&rmt_config, &g_rmt_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(g_tx_chan));

    // Configure the TX transmition
    g_tx_config.loop_count = 0;

    // Create message queue
    rmt_app_message_queue_handle = xQueueCreate(RMT_APP_MAX_QUEUE_SIZE, sizeof(rmt_app_message_t));

    // Start RMT Application task
    xTaskCreatePinnedToCore(
        &rmt_app_task,
        "rmt_app_task",
        RMT_APP_TASK_STACK_SIZE,
        NULL,
        RMT_APP_TASK_PRIORITY,
        NULL,
        RMT_APP_TASK_CORE_ID
    );

    // Start RMT Application's message queue task
    xTaskCreatePinnedToCore(
        &rmt_app_msg_queue_task,
        "rmt_app_msg_queue_task",
        RMT_APP_MSG_QUEUE_TASK_STACK_SIZE,
        NULL,
        RMT_APP_MSG_QUEUE_TASK_PRIORITY,
        NULL,
        RMT_APP_MSG_QUEUE_TASK_CORE_ID
    );
}

/**
 * Sends message to the RMT Application's message queue
 * @param msgID message ID
 */
void rmt_app_send_message(rmt_app_msg_e msgID) {
    rmt_app_message_t msg = {.msgID = msgID};
    xQueueSend(rmt_app_message_queue_handle, &msg, portMAX_DELAY);
}
