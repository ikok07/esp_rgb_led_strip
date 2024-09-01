//
// Created by kok on 01.09.24.
//

#ifndef LED_ENCODER_H
#define LED_ENCODER_H

#include "driver/rmt_encoder.h"

typedef struct {
    uint32_t resolution;
} rmt_led_strip_encoder_config_t;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

/**
 * Creates a new RMT LED Encoder
 * @param config led strip encoder structure
 * @param ret_encoder encoder handle which the function should set
 * @return ESP_OK if no error
 */
esp_err_t rmt_new_led_strip_encoder(const rmt_led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#endif //LED_ENCODER_H
