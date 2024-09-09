//
// Created by kok on 08.09.24.
//

#include "mqtt_client.h"
#include "esp_log.h"

#include "tasks_common.h"
#include "mqtt_app.h"

static const char TAG[] = "mqtt_app";

static esp_mqtt_client_handle_t mqtt_handle;

/**
 * Handle recieved data from the broker
 * @param event event handle which contains the published data received from the broker
 */
static void mqtt_app_handle_recv_data(esp_mqtt_event_handle_t event) {
    char buffer[MQTT_APP_BROKER_MAX_MSG_SIZE];
    snprintf(buffer, MQTT_APP_BROKER_MAX_MSG_SIZE, event->data, event->data_len);
    if (event->data_len >= MQTT_APP_BROKER_MAX_MSG_SIZE) {
        ESP_LOGE(TAG, "Buffer overflow! MQTT message could not be stored!");
        return;
    }
    buffer[event->data_len] = '\0';
    ESP_LOGI(TAG, "%s", buffer);
}

/**
 * MQTT method to handle event loop's events
 */
static void mqtt_app_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    const esp_mqtt_event_handle_t event_handle = event_data;
    switch (event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // Subscribe to topic
            signed int subscribe_flag;
            if ((subscribe_flag = esp_mqtt_client_subscribe_single(mqtt_handle, MQTT_APP_TOPIC, MQTT_APP_QOS)) < 0) {
                ESP_LOGE(TAG, "Failed to subscribe to topic: %s:\nError: %d", MQTT_APP_TOPIC, subscribe_flag);
                break;
            }
            ESP_LOGI(TAG, "Subscribed to topic %s with QOS: %d", MQTT_APP_TOPIC, MQTT_APP_QOS);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            mqtt_app_handle_recv_data(event_handle);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR: %d", event_handle->error_handle->error_type);
            break;
    }
}

void mqtt_app_init(void) {
    ESP_LOGI(TAG, "Configuring MQTT Application...");

    // Configure and initialize the MQTT's handle
    const esp_mqtt_client_config_t mqtt_config = {
        .broker = {
            .address = {
                .hostname = MQTT_APP_BROKER_HOST,
                .port = MQTT_APP_BROKER_PORT,
                .transport = MQTT_TRANSPORT_OVER_TCP,
            },
        },
        .credentials = {
            .username = MQTT_APP_USERNAME,
            .authentication = {
                .password = MQTT_APP_PASSWORD
            },
            .client_id = MQTT_APP_CLIENT_ID,
            .set_null_client_id = false,
        },
        .session = {
            .protocol_ver = MQTT_PROTOCOL_V_3_1_1,
            .last_will = {
                .topic = MQTT_APP_TOPIC,
                .msg = MQTT_APP_LAST_WILL_MSG,
                .qos = MQTT_APP_QOS,
                .retain = false,
            },
            .disable_clean_session = true
        },
        .task = {
            .stack_size = MQTT_APP_TASK_STACK_SIZE,
            .priority = MQTT_APP_TASK_PRIORITY
        },
    };
    mqtt_handle = esp_mqtt_client_init(&mqtt_config);

    // Register MQTT's event handlers
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_handle, MQTT_EVENT_ANY, mqtt_app_event_handler, NULL));

    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_handle));

    ESP_LOGI(TAG, "MQTT Application successfully initialized!");
}
