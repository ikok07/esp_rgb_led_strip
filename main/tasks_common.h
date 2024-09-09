//
// Created by kok on 02.09.24.
//

#ifndef TASKS_COMMON_H
#define TASKS_COMMON_H

// --------- CORE 0 --------- //

#define RMT_APP_TASK_PRIORITY                 5
#define RMT_APP_TASK_STACK_SIZE               4096
#define RMT_APP_TASK_CORE_ID                  0

// MQTT's Core Id is configured in menuconfig
#define MQTT_APP_TASK_PRIORITY                4
#define MQTT_APP_TASK_STACK_SIZE              4096

#define WIFI_APP_TASK_PRIORITY                3
#define WIFI_APP_TASK_STACK_SIZE              8192
#define WIFI_APP_TASK_CORE_ID                 1

// --------- CORE 1 --------- //

#define RMT_APP_MSG_QUEUE_TASK_PRIORITY       5
#define RMT_APP_MSG_QUEUE_TASK_STACK_SIZE     4096
#define RMT_APP_MSG_QUEUE_TASK_CORE_ID        0

#define MODE_SWITCHER_TASK_PRIORITY           4
#define MODE_SWITCHER_TASK_STACK_SIZE         2048
#define MODE_SWITCHER_TASK_CORE_ID            1

#define OBJECT_SENSOR_TASK_PRIORITY           4
#define OBJECT_SENSOR_TASK_STACK_SIZE         2048
#define OBJECT_SENSOR_TASK_CORE_ID            1

#define HTTP_SERVER_TASK_PRIORITY             3
#define HTTP_SERVER_TASK_STACK_SIZE           8192
#define HTTP_SERVER_TASK_CORE_ID              1

#endif //TASKS_COMMON_H
