//
// Created by kok on 08.09.24.
//

#include "mqtt_client.h"
#include "esp_log.h"

#include "tasks_common.h"
#include "cjson/cJSON.h"
#include "mqtt_app.h"

#include "rmt/rmt_app.h"

static const char TAG[] = "mqtt_app";

static esp_mqtt_client_handle_t mqtt_handle;

static void mqtt_app_publish_task(void *pvParams) {
    ESP_LOGI(TAG, "Start publishing current LED config to MQTT broker");
    while(1) {
        const rmt_app_active_config_t led_config = rmt_app_get_active_config();

        cJSON *json = cJSON_CreateObject();
        if (json == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object!");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        cJSON_AddStringToObject(json, "tag", MQTT_APP_TAG_LED_STRIP);
        cJSON_AddNumberToObject(json, "state", led_config.state);
        cJSON_AddNumberToObject(json, "mode", led_config.mode);

        cJSON *color = cJSON_CreateObject();
        if (color == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object for field \"color\"!");
            cJSON_Delete(json);  // Clean up the main object
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        cJSON_AddNumberToObject(color, "red", led_config.colors.red);
        cJSON_AddNumberToObject(color, "green", led_config.colors.green);
        cJSON_AddNumberToObject(color, "blue", led_config.colors.blue);

        cJSON_AddItemToObject(json, "color", color);

        char *json_str = cJSON_Print(json);
        if (json_str == NULL) {
            ESP_LOGE(TAG, "Failed to print JSON object!");
            cJSON_Delete(json);  // Clean up the main object
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        int result;
        if ((result = esp_mqtt_client_publish(mqtt_handle, MQTT_APP_PUBLISH_TOPIC, json_str, strlen(json_str), MQTT_APP_QOS, false)) < 0) {
            ESP_LOGE(TAG, "Failed to publish message to MQTT broker! Error code: %d", result);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            return;
        }

        cJSON_free(json_str);
        cJSON_Delete(json);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

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
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON MQTT message!");
        return;
    }
    cJSON *tag = cJSON_GetObjectItemCaseSensitive(json, "tag");
    if (tag == NULL || tag->valuestring == NULL) {
        ESP_LOGE(TAG, "Tag field missing from JSON!");
        return;
    }

    // Run specific task depending on the provided tag
    if (strcmp(tag->valuestring, MQTT_APP_TAG_LED_STRIP) == 0) {
        rmt_app_set_from_json(json);
    }

    free(json);
    free(tag);
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
            if ((subscribe_flag = esp_mqtt_client_subscribe_single(mqtt_handle, MQTT_APP_SUBSCRIBE_TOPIC, MQTT_APP_QOS)) < 0) {
                ESP_LOGE(TAG, "Failed to subscribe to topic: %s:\nError: %d", MQTT_APP_SUBSCRIBE_TOPIC, subscribe_flag);
                break;
            }
            ESP_LOGI(TAG, "Subscribed to topic %s with QOS: %d", MQTT_APP_SUBSCRIBE_TOPIC, MQTT_APP_QOS);
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
                .topic = MQTT_APP_PUBLISH_TOPIC,
                .msg = MQTT_APP_LAST_WILL_MSG,
                .msg_len = strlen(MQTT_APP_LAST_WILL_MSG),
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

    xTaskCreatePinnedToCore(
        &mqtt_app_publish_task,
        "mqtt_app_publish_task",
        MQTT_APP_TASK_STACK_SIZE,
        NULL,
        MQTT_APP_TASK_PRIORITY,
        NULL,
        MQTT_APP_TASK_CORE_ID
    );

    ESP_LOGI(TAG, "MQTT Application successfully initialized!");
}
