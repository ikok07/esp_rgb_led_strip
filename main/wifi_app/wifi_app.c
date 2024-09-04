//
// Created by kok on 04.09.24.
//


#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"

#include "esp_netif.h"
#include "tasks_common.h"
#include "wifi_app.h"

static const char TAG[] = "wifi_app";

static QueueHandle_t wifi_app_message_queue_handle = NULL;

static esp_netif_t *wifi_app_ap = NULL;
static esp_netif_t *wifi_app_sta = NULL;

/**
 * WiFi application message queue task
 */
static void wifi_app_msg_queue_task(void *pvParams) {
    wifi_app_message_t msg;
    while(1) {
        if (xQueueReceive(wifi_app_message_queue_handle, &msg, portMAX_DELAY)) {
            switch (msg.msgID) {
                case WIFI_APP_MSG_CONNECT:
                    break;
                case WIFI_APP_MSG_DISCONNECT:
                    break;
            }
        }
    }
}

/**
 * Start the WiFi driver
 */
static void wifi_app_start_driver(void) {
    ESP_LOGI(TAG, "Starting WiFi driver");
    // Create a LwIP core task
    ESP_ERROR_CHECK(esp_netif_init());

    //Create a system Event task
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default network interfaces.
    wifi_app_ap = esp_netif_create_default_wifi_ap();
    wifi_app_sta = esp_netif_create_default_wifi_sta();

    // Initialize the WiFi
    const wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_LOGI(TAG, "WiFi driver successfully started!");
}

/**
 * Configure the ESP32's AP
 */
static void wifi_app_ap_configure(void) {
    ESP_LOGI(TAG, "Configuring access point");

    // Configure the DHCP
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = esp_ip4addr_aton(WIFI_APP_AP_IP_ADDRESS);
    ip_info.gw.addr = esp_ip4addr_aton(WIFI_APP_AP_DEFAULT_GATEWAY);
    ip_info.netmask.addr = esp_ip4addr_aton(WIFI_APP_AP_NETMASK);

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(wifi_app_ap));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_app_ap, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(wifi_app_ap));

    // Configure the ESP32's AP settings
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_APP_AP_SSID,
            .password = WIFI_APP_AP_PASSWORD,
            .ssid_len = WIFI_APP_AP_SSID_LENGTH,
            .channel = WIFI_APP_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA3_PSK,
            .max_connection = WIFI_APP_AP_MAX_CONNECTIONS,
            .beacon_interval = 100,
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    ESP_LOGI(TAG, "Access point successfully configured!");
}

void wifi_app_init(void) {
    // Start the WiFi driver
    wifi_app_start_driver();

    // Configure the ESP32's AP
    wifi_app_ap_configure();

    // Start the WiFi driver
    esp_wifi_start();

    esp_log_level_set("wifi", ESP_LOG_ERROR);

    wifi_app_message_queue_handle = xQueueCreate(3, sizeof(wifi_app_message_t));

    xTaskCreatePinnedToCore(
        &wifi_app_msg_queue_task,
        "wifi_app_msg_queue_task",
        WIFI_APP_TASK_STACK_SIZE,
        NULL,
        WIFI_APP_TASK_PRIORITY,
        NULL,
        WIFI_APP_TASK_CORE_ID
    );
}


// .sta = {
//     .ssid = WIFI_APP_STA_SSID,
//     .password = WIFI_APP_STA_PASSWORD,
//     .scan_method = WIFI_ALL_CHANNEL_SCAN,
//     .channel = WIFI_APP_STA_CHANNEL,
//     .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
// }