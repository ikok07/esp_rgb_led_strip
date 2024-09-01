//
// Created by kok on 31.08.24.
//

#include <esp_log.h>

#include "driver/rmt_encoder.h"

#ifndef RMT_APP_H
#define RMT_APP_H

#define RMT_APP_SRC_CLK                 RMT_CLK_SRC_DEFAULT
#define RMT_APP_GPIO_NUM                0
#define RMT_APP_MEM_BLOCK_SYMBOLS       64
#define RMT_APP_RESOLUTION_HZ           1 * 1000 * 1000 // 1MHz; 1 tick == 1 µs
#define RMT_APP_TRANS_QUEUE_SIZE        4

#define RMT_APP_LED_NUMBERS             60
#define RMT_APP_LED_CHASE_SPEED         10

/**
 * Initialized the rmt application
 */
void rmt_app_start(void);

#endif //RMT_APP_H
