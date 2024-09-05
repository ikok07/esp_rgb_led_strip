//
// Created by kok on 04.09.24.
//

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi_default.h"
#include "nvs.h"

#include "esp_netif.h"
#include "tasks_common.h"
#include "wifi_app.h"

static const char TAG[] = "wifi_app";
static const char NVS_NAMESPACE[] = "wifi_cred";

static QueueHandle_t wifi_app_message_queue_handle = NULL;
static EventGroupHandle_t wifi_app_event_group_handle = NULL;

static uint32_t WIFI_APP_USER_DICONNECT_ATTEMPT         = BIT0;
static uint32_t WIFI_APP_CONNECT_WITH_STORED_CREDS      = BIT1;

static wifi_config_t g_wifi_sta_config;
static uint8_t g_wifi_app_retry_count = 0;

static esp_netif_t *wifi_app_ap = NULL;
static esp_netif_t *wifi_app_sta = NULL;

// --------- STA CONNECTION --------- //

esp_err_t wifi_app_sta_scan(wifi_app_sta_scan_results_t *results) {
    const wifi_scan_config_t config = {
        .show_hidden = true,
    };
    esp_err_t err = esp_wifi_scan_start(&config, true);
    if (err != ESP_OK) return err;
    uint16_t max_records = WIFI_APP_STA_MAX_AP_RECORDS;
    wifi_ap_record_t *records = malloc(sizeof(wifi_ap_record_t) * max_records);
    err = esp_wifi_scan_get_ap_records(&max_records, records);
    if (err != ESP_OK) return err;
    results->records_count = max_records;
    results->records = records;
    return ESP_OK;
}

/**
 * Connect to a remote AP
 */
static void wifi_app_sta_connect() {
    esp_err_t err = esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_sta_config());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to remote AP! %s", esp_err_to_name(err));
        return;
    }
    err = esp_wifi_connect();
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to connect to remote AP! %s", esp_err_to_name(err));
}

// --------- NON-VOLATILE STORAGE --------- //

/**
 * Save current remote AP's credentials to the NVS.
 */
static esp_err_t wifi_app_sta_save_creds() {
    nvs_handle_t nvs_handle;
    const wifi_config_t *config = wifi_app_get_sta_config();
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for storing the AP's credentials! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, "ssid", (char*)config->sta.ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store remote AP's credentials in the flash! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    err = nvs_set_str(nvs_handle, "pass", (char*)config->sta.password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store remote AP's credentials in the flash! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store remote AP's credentials in the flash! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Successfully saved remote AP's credentials in the flash!");
    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * Load last remote AP's credentials from the NVS.
 */
static esp_err_t wifi_app_sta_load_creds() {
    ESP_LOGI(TAG, "Loading WIFI credentials from flash...");
    wifi_config_t *config = wifi_app_get_sta_config();
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for storing the AP's credentials! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    size_t ssid_size = sizeof(config->sta.ssid);
    err = nvs_get_str(nvs_handle, "ssid", (char*)config->sta.ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to retrieve remote AP's ssid from the flash! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    ESP_LOGI(TAG, "Stored SSID: %s", config->sta.ssid);

    size_t pass_size = sizeof(config->sta.password);
    err = nvs_get_str(nvs_handle, "pass", (char*)config->sta.password, &pass_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to retrieve remote AP's password from the flash! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Stored password: %s", config->sta.password);

    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * Remove last remote AP's credentials from the NVS.
 */
static esp_err_t wifi_app_sta_remove_creds() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for removing the AP's credentials! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_erase_key(nvs_handle, "wifi");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase AP's stored credentials from NVS! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit changes after removing AP's credentials from the store! %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

// --------- EVENT AND MESSAGE HANDLERS --------- //

/**
 * WiFi application message queue task
 */
static void wifi_app_msg_queue_task(void *pvParams) {
    wifi_app_message_t msg;
    EventBits_t event_bits;

    while(1) {
        if (xQueueReceive(wifi_app_message_queue_handle, &msg, portMAX_DELAY)) {
            switch (msg.msgID) {
                case WIFI_APP_MSG_CONNECT:
                    event_bits = xEventGroupGetBits(wifi_app_event_group_handle);
                    ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECT");

                    // Load credentials if option to get them from NVS is specified
                    if (event_bits & WIFI_APP_CONNECT_WITH_STORED_CREDS) {
                        ESP_LOGI(TAG, "WIFI_APP_CONNECT_WITH_STORED_CREDS");
                        const esp_err_t err = wifi_app_sta_load_creds();
                        if (err != ESP_OK) {
                            xEventGroupClearBits(wifi_app_event_group_handle, WIFI_APP_CONNECT_WITH_STORED_CREDS);
                            continue;
                        }
                    }

                    wifi_app_sta_connect();
                    break;
                case WIFI_APP_MSG_DISCONNECT:
                    event_bits = xEventGroupGetBits(wifi_app_event_group_handle);
                    if (event_bits & WIFI_APP_USER_DICONNECT_ATTEMPT) {
                        xEventGroupClearBits(wifi_app_event_group_handle, WIFI_APP_USER_DICONNECT_ATTEMPT);
                        // Clear the NVS storage
                        wifi_app_sta_remove_creds();
                    }
                    break;
                default: break;
            }
        }
    }
}

/**
 * WiFi application event handler
 */
static void wifi_app_event_handler(void *arg, const esp_event_base_t event_base, const int32_t event_id, void *event_data) {
    EventBits_t event_bits;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
                event_bits = xEventGroupGetBits(wifi_app_event_group_handle);

                wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = malloc(sizeof(wifi_event_sta_disconnected_t));
                *wifi_event_sta_disconnected = *(wifi_event_sta_disconnected_t*)event_data;
                ESP_LOGI(TAG, "WIFI disconnect reason code %d", wifi_event_sta_disconnected->reason);

                if ((event_bits & WIFI_APP_USER_DICONNECT_ATTEMPT) || g_wifi_app_retry_count >= WIFI_APP_STA_MAX_RETRIES) {
                    wifi_app_send_message(WIFI_APP_MSG_DISCONNECT, NULL);
                    g_wifi_app_retry_count = 0;
                } else {
                    ESP_LOGI(TAG, "WiFi retrying connection... (Attempt: %d)", g_wifi_app_retry_count + 1);
                    wifi_app_sta_connect();
                    g_wifi_app_retry_count++;
                }

                free(wifi_event_sta_disconnected);
                break;
            default: break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "Successfully connected to remote AP!");
                // Save credentials to NVS if option is selected
                event_bits = xEventGroupGetBits(wifi_app_event_group_handle);
                if (event_bits & WIFI_APP_CONNECT_WITH_STORED_CREDS) {
                    const esp_err_t err = wifi_app_sta_save_creds();
                    if (err == ESP_OK) xEventGroupClearBits(wifi_app_event_group_handle, WIFI_APP_CONNECT_WITH_STORED_CREDS);
                }

                break;
            default: break;
        }
    }
}

// --------- WIFI INITIAL CONFIGURATIONS --------- //

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
    wifi_config_t ap_config = {
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
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_APP_AP_BANDWIDTH));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, NULL));

    ESP_LOGI(TAG, "Access point successfully configured!");
}

void wifi_app_init(void) {
    // Start the WiFi driver
    wifi_app_start_driver();

    // Configure the ESP32's AP
    wifi_app_ap_configure();

    // Start the WiFi driver
    esp_wifi_start();

    // Disable verbose logging
    esp_log_level_set("wifi", ESP_LOG_ERROR);

    // Create message queue and event group
    wifi_app_message_queue_handle = xQueueCreate(3, sizeof(wifi_app_message_t));
    wifi_app_event_group_handle = xEventGroupCreate();

    // Configure STA mode
    g_wifi_sta_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    g_wifi_sta_config.sta.channel = WIFI_APP_STA_CHANNEL;

    // TEMPORARY
    // memcpy(wifi_app_get_sta_config()->sta.ssid, "", strlen("") + 1);
    // memcpy(wifi_app_get_sta_config()->sta.password, "", strlen("") + 1);
    // wifi_app_send_message(WIFI_APP_MSG_CONNECT, NULL);
    xEventGroupSetBits(wifi_app_event_group_handle, WIFI_APP_CONNECT_WITH_STORED_CREDS);
    wifi_app_send_message(WIFI_APP_MSG_CONNECT, NULL);

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

void wifi_app_send_message(const wifi_app_msg_e msgID, void *pvParams) {
    const wifi_app_message_t msg = {.msgID = msgID, .params = pvParams};
    xQueueSend(wifi_app_message_queue_handle, &msg, portMAX_DELAY);
}

wifi_config_t *wifi_app_get_sta_config(void) {
    return &g_wifi_sta_config;
}