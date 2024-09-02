//
// Created by kok on 01.09.24.
//

#include "esp_log.h"

#include "led_encoder.h"

static const char TAG[] = "led_encoder";

/**
 *
 * @param[in] encoder Encoder handle
 * @param[in] tx_channel RMT TX channel handle, returned from `rmt_new_tx_channel()`
 * @param[in] primary_data App data to be encoded into RMT symbols
 * @param[in] data_size Size of primary_data, in bytes
 * @param[out] ret_state Returned current encoder's state
 * @return Number of RMT symbols that the primary data has been encoded into
 */
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
        case RMT_ENCODING_RESET: // Send RGB data
            encoded_symbols += bytes_encoder->encode(bytes_encoder, tx_channel, primary_data, data_size, &session_state);
            if (session_state & RMT_ENCODING_COMPLETE) {
                led_encoder->state = RMT_ENCODING_COMPLETE;
            }
            if (session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
                goto out;
            }
            break;
        case RMT_ENCODING_COMPLETE:
            encoded_symbols += copy_encoder->encode(copy_encoder, tx_channel, &led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);
            if (session_state & RMT_ENCODING_COMPLETE) {
                led_encoder->state = RMT_ENCODING_RESET;
                state |= RMT_ENCODING_COMPLETE;
            }
            if (session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
                goto out;
            }
            break;
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

/**
 * Delets the LED strip's encoder
 * @return ESP_OK if no errors occurred
 */
static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

/**
 * Resets the LED strip's encoder
 * @return ESP_OK if no errors occurred
 */
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t rmt_new_led_strip_encoder(const rmt_led_strip_encoder_config_t *config, rmt_encoder_handle_t*ret_encoder) {
    esp_err_t err = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;

    if (!(config && ret_encoder)) {
        ESP_LOGE(TAG, "Encoder could not be created. Invalid arguments passed to function!");
        return ESP_ERR_INVALID_ARG;
    }

    led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
    if (led_encoder == NULL) {
        ESP_LOGE(TAG, "Could not allocate enough memory for rmt's LED encoder!");
        return ESP_ERR_NO_MEM;
    }

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    const rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 0.3 * config->resolution / 1000000, // T0H = 0.3us
            .level1 = 0,
            .duration1 = 0.9 * config->resolution / 1000000, // T0L = 0.9us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 0.9 * config->resolution / 1000000, // T1H = 0.9us
            .level1 = 0,
            .duration1 = 0.3 * config->resolution / 1000000, // T1L = 0.3us
        },
        .flags.msb_first = 1,
    };
    err = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create bytes encoder for rmt application!");
        return err;
    }

    const rmt_copy_encoder_config_t copy_encoder_config = {};
    err = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create copy encoder for rmt application!");
        return err;
    }

    uint32_t reset_ticks = config->resolution / 1000000 * 50 / 2; // reset code duration => 50us
    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };
    *ret_encoder = &led_encoder->base;
    return ESP_OK;
}