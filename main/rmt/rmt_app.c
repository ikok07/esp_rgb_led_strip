//
// Created by kok on 31.08.24.
//

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "portmacro.h"
#include "nvs.h"

#include "led_encoder/led_encoder.h"
#include "tasks_common.h"
#include "rmt_app.h"


/**
 * RMT Applicatiom messaging queue handle
 */
QueueHandle_t rmt_app_message_queue_handle = NULL;

static const char TAG[] = "rmt_app";
static const char NVS_NAMESPACE[] = "rmt_app";

static rmt_channel_handle_t g_tx_chan = NULL;
static rmt_encoder_handle_t g_rmt_encoder = NULL;
static rmt_transmit_config_t g_tx_config;
static rmt_app_state_e g_rmt_app_state = RMT_APP_LED_OFF;
static rmt_app_mode_e g_rmt_app_sel_mode = RMT_APP_LED_MODE_RAINBOW;

static uint8_t g_red_value = 255;
static uint8_t g_green_value = 0;
static uint8_t g_blue_value = 0;


// --------- AUX RMT METHODS --------- //

/**
 * Convert HSV values to standard RGB for the LED strip
 */
static void led_strip_hsv2rgb(uint32_t hue, const uint32_t saturation, const uint32_t value, uint8_t *red, uint8_t *green, uint8_t *blue) {
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

// --------- NVS STORAGE --------- //

static void rmt_app_save_config_to_flash() {
    ESP_LOGI(TAG, "Saving RMT configuration to NVS...");
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to open NVS in order to save RMT configuration! %s", esp_err_to_name(err));

    err = nvs_set_u8(nvs_handle, "state", g_rmt_app_state);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set rmt state in NVS: %s", esp_err_to_name(err));

    err = nvs_set_u8(nvs_handle, "mode", g_rmt_app_sel_mode);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set rmt mode in NVS: %s", esp_err_to_name(err));

    err = nvs_set_u8(nvs_handle, "red", g_red_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set red value in NVS: %s", esp_err_to_name(err));

    err = nvs_set_u8(nvs_handle, "green", g_green_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set green value in NVS: %s", esp_err_to_name(err));

    err = nvs_set_u8(nvs_handle, "blue", g_blue_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set blue value in NVS: %s", esp_err_to_name(err));

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to commit RMT configuration to NVS: %s", esp_err_to_name(err));

    nvs_close(nvs_handle);
}

static void rmt_app_get_config_from_flash() {
    ESP_LOGI(TAG, "Fetching RMT configuration from NVS...");
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to open NVS in order to save RMT configuration! %s", esp_err_to_name(err));

    err = nvs_get_u8(nvs_handle, "state", (uint8_t*)&g_rmt_app_state);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to get rmt state from NVS: %s", esp_err_to_name(err));

    err = nvs_get_u8(nvs_handle, "mode", (uint8_t*)&g_rmt_app_sel_mode);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to get red value from NVS: %s", esp_err_to_name(err));

    err = nvs_get_u8(nvs_handle, "red", &g_red_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to get red value from NVS: %s", esp_err_to_name(err));

    err = nvs_get_u8(nvs_handle, "green", &g_green_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to get green value from NVS: %s", esp_err_to_name(err));

    err = nvs_get_u8(nvs_handle, "blue", &g_blue_value);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to get blue value from NVS: %s", esp_err_to_name(err));

    nvs_close(nvs_handle);

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
                    if (g_rmt_app_state == RMT_APP_LED_ON) {
                        ESP_LOGI(TAG, "LED turned OFF");
                        g_rmt_app_state = RMT_APP_LED_OFF;
                    } else {
                        g_rmt_app_state = RMT_APP_LED_ON;
                        ESP_LOGI(TAG, "Selected LED mode: %d", g_rmt_app_sel_mode);
                    }
                    break;
                case RMT_APP_MSG_CYCLE_MODE:
                    if (g_rmt_app_state == RMT_APP_LED_OFF) {
                        ESP_LOGI(TAG, "LED mode could not be changed because it's turned OFF");
                        continue;
                    }
                    if (g_rmt_app_sel_mode == RMT_APP_LED_MODES_COUNT - 1) g_rmt_app_sel_mode = RMT_APP_LED_MODE_RAINBOW;
                    else g_rmt_app_sel_mode++;
                    ESP_LOGI(TAG, "Selected LED Mode: %d", g_rmt_app_sel_mode);
            }
            rmt_app_save_config_to_flash();
        }
    }
}

// --------- TRANSMIT RMT DATA --------- //

/**
 *  Creates a new RMT transmit configuration which is suited when using hue value
 * @return RMT transmit configuration
 */
static rmt_app_transmit_hue_config_t *rmt_app_new_transmit_hue_config(
    const uint32_t hue,
    const uint8_t *saturation,
    const uint8_t *value,
    const uint32_t *start_rgb
) {
    rmt_app_transmit_hue_config_t *config = malloc(sizeof(rmt_app_transmit_hue_config_t));
    if (config == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for rmt_app_transmit_config_t!");
        abort();
    }

    config->hue = hue;

    if (saturation != NULL) config->saturation = *saturation;
    else config->saturation = 100;

    if (value != NULL) config->saturation = *value;
    else config->value = 100;

    if (start_rgb != NULL) config->start_rgb = *start_rgb;
    else config->start_rgb = 0;

    return config;
}

/**
 *  Creates a new RMT transmit configuration which is suited when using raw RGB values
 * @return RMT transmit configuration
 */
static rmt_app_transmit_config_t *rmt_app_new_transmit_config(
    const uint8_t red,
    const uint8_t green,
    const uint8_t blue
) {
    rmt_app_transmit_config_t *config = malloc(sizeof(rmt_app_transmit_config_t));
    if (config == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for rmt_app_transmit_config_t!");
        abort();
    }

    config->red = red;
    config->green = green;
    config->blue = blue;

    return config;
}

/**
 * Transmit the rmt data to the LED\n
 * @warning YOU SHOULD PASS ONLY ONE CONFIGURATION STRUCTURE TO THE METHOD
 * @param config RMT Application transmit configuration structure suitable when using raw RGB values
 * @param hue_config RMT Application transmit configuration structure suitable when using hue value
 */
static void rmt_app_transmit_data(rmt_app_transmit_config_t *config, rmt_app_transmit_hue_config_t *hue_config) {
    if (config == NULL && hue_config == NULL) {
        ESP_LOGE(TAG, "Unable to transfer data: No config provided!");
        return;
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t led_strip_pixels[RMT_APP_LED_NUMBERS * 3] = {0};
    for (int i = 0; i < RMT_APP_LED_NUMBERS * 3; i += 3) {
        // Build RGB pixels
        if (hue_config != NULL) {
            hue_config->hue = i * 360 / RMT_APP_LED_NUMBERS;
            hue_config->hue += hue_config->start_rgb;
            led_strip_hsv2rgb(hue_config->hue, hue_config->saturation, hue_config->value, &red, &green, &blue);
        } else {
            red = config->red;
            green = config->green;
            blue = config->blue;
        }

        led_strip_pixels[i + 0] = green;
        led_strip_pixels[i + 1] = red;
        led_strip_pixels[i + 2] = blue;
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(g_tx_chan, g_rmt_encoder, led_strip_pixels, RMT_APP_LED_NUMBERS * 3, &g_tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(g_tx_chan, portMAX_DELAY));
}

// --------- RMT LED MODE METHODS --------- //

/**
 * Turns the LED off
 */
static void rmt_app_led_off(void) {
    rmt_app_transmit_config_t *config = rmt_app_new_transmit_config(0, 0, 0);
    rmt_app_transmit_data(config, NULL);
    free(config);
}

static void rmt_app_led_mode_rainbow(void) {
    uint32_t hue = 0;
    static uint32_t start_rgb = 0;
    for (int i = 0; i < 3; i++) {
        rmt_app_transmit_hue_config_t *hue_config = rmt_app_new_transmit_hue_config(hue, NULL, NULL, &start_rgb);
        rmt_app_transmit_data(NULL, hue_config);
        vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
        rmt_app_transmit_data(NULL, hue_config);
        vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
        free(hue_config);
    }
    start_rgb += 60;
}

static void rmt_app_led_mode_static(void) {
    rmt_app_transmit_config_t *config = rmt_app_new_transmit_config(g_red_value, g_green_value, g_blue_value);
    rmt_app_transmit_data(config, NULL);
    vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
    free(config);
}

// --------- MAIN RMT METHODS --------- //

/**
 * RMT Application task
 */
static void rmt_app_task(void *pvParams) {
    while (1) {
        if (g_rmt_app_state == RMT_APP_LED_ON) {
            switch (g_rmt_app_sel_mode) {
                case RMT_APP_LED_MODE_RAINBOW:
                    rmt_app_led_mode_rainbow();
                break;
                case RMT_APP_LED_MODE_STATIC:
                    rmt_app_led_mode_static();
                break;
            }
        } else rmt_app_led_off();
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

    // Fetch saved configuration from NVS
    rmt_app_get_config_from_flash();

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


void rmt_app_send_message(rmt_app_msg_e msgID) {
    rmt_app_message_t msg = {.msgID = msgID};
    xQueueSend(rmt_app_message_queue_handle, &msg, portMAX_DELAY);
}


void rmt_app_set_rgb_color(uint8_t r, uint8_t g, uint8_t b) {
    g_red_value = r;
    g_green_value = g;
    g_blue_value = b;
    rmt_app_save_config_to_flash();
}

void rmt_app_set_from_json(cJSON *json) {
    const cJSON *state = cJSON_GetObjectItemCaseSensitive(json, "state");
    if (state == NULL || !cJSON_IsNumber(state) || state->valueint < 0 || state->valueint > 1)
        ESP_LOGE(TAG, "Missing or invalid state provided by JSON!");
    else g_rmt_app_state = state->valueint;

    // Check if LED was turned off
    if (g_rmt_app_state == RMT_APP_LED_OFF) return;

    const cJSON *mode = cJSON_GetObjectItemCaseSensitive(json, "mode");
    if (mode == NULL || !cJSON_IsNumber(mode) || mode->valueint < 0 || mode->valueint > RMT_APP_LED_MODES_COUNT - 1)
        ESP_LOGE(TAG, "Missing or invalid mode provided by JSON!");
    else g_rmt_app_sel_mode = mode->valueint;

    // Check if LED mode was set rainbow
    if (g_rmt_app_sel_mode == RMT_APP_LED_MODE_RAINBOW) return;

    const cJSON *color_json = cJSON_GetObjectItemCaseSensitive(json, "color");
    if (color_json == NULL) {
        ESP_LOGE(TAG, "Missing or invalid color object provided by JSON!");
        return;
    }

    const cJSON *red = cJSON_GetObjectItemCaseSensitive(color_json, "red");
    if (red == NULL || !cJSON_IsNumber(red) || red->valueint < 0 || red->valueint > 255)
        ESP_LOGE(TAG, "Missing or invalid red value provided by JSON!");
    else g_red_value = red->valueint;

    const cJSON *green = cJSON_GetObjectItemCaseSensitive(color_json, "green");
    if (green == NULL || !cJSON_IsNumber(green) || green->valueint < 0 || green->valueint > 255)
        ESP_LOGE(TAG, "Missing or invalid green value provided by JSON!");
    else g_green_value = green->valueint;

    const cJSON *blue = cJSON_GetObjectItemCaseSensitive(color_json, "blue");
    if (blue == NULL || !cJSON_IsNumber(blue) || blue->valueint < 0 || blue->valueint > 255)
        ESP_LOGE(TAG, "Missing or invalid blue value provided by JSON!");
    else g_blue_value = blue->valueint;
}
