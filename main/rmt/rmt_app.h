//
// Created by kok on 31.08.24.
//

#include <esp_log.h>

#include "driver/rmt_encoder.h"

#ifndef RMT_APP_H
#define RMT_APP_H

#define RMT_APP_SRC_CLK                       RMT_CLK_SRC_DEFAULT
#define RMT_APP_LED_GPIO_NUM                  0
#define RMT_APP_MEM_BLOCK_SYMBOLS             64
#define RMT_APP_RESOLUTION_HZ                 1 * 1000 * 1000 // 1MHz; 1 tick == 1 µs
#define RMT_APP_TRANS_QUEUE_SIZE              4

#define RMT_APP_LED_NUMBERS                   60
#define RMT_APP_LED_CHASE_SPEED               10

#define RMT_APP_MAX_QUEUE_SIZE                3

/**
 * Types of LED strip modes
 */
typedef enum {
    RMT_APP_LED_OFF,
    RMT_APP_LED_MODE1,
} rmt_app_mode_e;

/**
 * RMT Application messages enum
 */
typedef enum {
    RMT_APP_MSG_TOGGLE_LED
} rmt_app_msg_e;

/**
 * RMT Application message structure
 */
typedef struct {
    rmt_app_msg_e msgID;
} rmt_app_message_t;

/**
 * Transmit configuration
 */
typedef struct {
    rmt_channel_handle_t tx_chan;
    rmt_transmit_config_t tx_config;
    rmt_encoder_handle_t rmt_encoder;
    uint8_t led_strip_pixels[RMT_APP_LED_NUMBERS * 3] = {0};
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t *hue = NULL;
    uint32_t *start_rgb = NULL;
} rmt_app_transmit_config_t;

/**
 * Initialized the rmt application
 */
void rmt_app_start(void);

/**
 * Sends message to the RMT Application message queue
 * @param msgID rmt_app_msg_e (RMT Application message ID)
 */
void rmt_app_send_message(rmt_app_msg_e msgID);

#endif //RMT_APP_H
