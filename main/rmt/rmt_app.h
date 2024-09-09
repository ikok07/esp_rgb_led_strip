//
// Created by kok on 31.08.24.
//

#ifndef RMT_APP_H
#define RMT_APP_H

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

#define RMT_APP_SRC_CLK                       RMT_CLK_SRC_DEFAULT
#define RMT_APP_LED_GPIO_NUM                  27
#define RMT_APP_MEM_BLOCK_SYMBOLS             64
#define RMT_APP_RESOLUTION_HZ                 10 * 1000 * 1000 // 10MHz; 10 tick == 1 µs
#define RMT_APP_TRANS_QUEUE_SIZE              4

#define RMT_APP_LED_NUMBERS                   30
#define RMT_APP_LED_CHASE_SPEED               10

#define RMT_APP_MAX_QUEUE_SIZE                3

/**
 * Types of LED strip modes
 */
#define RMT_APP_LED_MODES_COUNT               3
typedef enum {
    RMT_APP_LED_OFF,
    RMT_APP_LED_MODE_RAINBOW,
    RMT_APP_LED_MODE_STATIC
} rmt_app_mode_e;

/**
 * RMT Application messages enum
 */
typedef enum {
    RMT_APP_MSG_TOGGLE_LED,
    RMT_APP_MSG_CYCLE_MODE
} rmt_app_msg_e;

/**
 * RMT Application message structure
 */
typedef struct {
    rmt_app_msg_e msgID;
} rmt_app_message_t;

/**
 * Transmit configuration suitable when using hue value
 */
typedef struct {
    uint32_t hue;
    uint8_t saturation;
    uint8_t value;
    uint32_t start_rgb;
} rmt_app_transmit_hue_config_t;

/**
 * Transmit configuration suitable when using raw RGB values
 */
typedef struct {
 uint8_t red;
 uint8_t green;
 uint8_t blue;
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

/**
 * Sets the RGB values
 * @param r red value
 * @param g green value
 * @param b blue value
 */
void rmt_app_set_rgb_color(uint8_t r, uint8_t g, uint8_t b);

#endif //RMT_APP_H
