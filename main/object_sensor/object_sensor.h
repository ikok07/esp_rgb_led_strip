//
// Created by kok on 01.09.24.
//

#ifndef OBJECT_SENSOR_H
#define OBJECT_SENSOR_H

#include "driver/adc.h"

#define OBJECT_SENSOR_GPIO                32

/**
 * Initialize the object_sensor task
 */
void object_sensor_init(void);

#endif //OBJECT_SENSOR_H
