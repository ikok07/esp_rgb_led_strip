//
// Created by kok on 31.08.24.
//

#include <string.h>

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"

#include "led_encoder/led_encoder.h"
#include "rmt_app.h"

static const char TAG[] = "rmt_app";
static uint8_t led_strip_pixels[RMT_APP_LED_NUMBERS * 3];

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

void rmt_app_start(void) {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint32_t hue = 0;
    uint32_t start_rgb = 0;

    // Configure and create RMT TX Channel
    const rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_APP_SRC_CLK,
        .gpio_num = RMT_APP_GPIO_NUM,
        .resolution_hz = RMT_APP_RESOLUTION_HZ,
        .mem_block_symbols = RMT_APP_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = RMT_APP_TRANS_QUEUE_SIZE
    };
    rmt_channel_handle_t tx_chan;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));

    // Configure and create the RMT Encoder
    rmt_encoder_handle_t rmt_encoder = NULL;
    const rmt_led_strip_encoder_config_t rmt_config = {
        .resolution = RMT_APP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&rmt_config, &rmt_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(tx_chan));

    const rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    while (1) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < RMT_APP_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                hue = j * 360 / RMT_APP_LED_NUMBERS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                led_strip_pixels[j * 3 + 0] = green;
                led_strip_pixels[j * 3 + 1] = blue;
                led_strip_pixels[j * 3 + 2] = red;
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(tx_chan, rmt_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
            ESP_ERROR_CHECK(rmt_transmit(tx_chan, rmt_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(RMT_APP_LED_CHASE_SPEED));
        }
        start_rgb += 60;
    }
}
