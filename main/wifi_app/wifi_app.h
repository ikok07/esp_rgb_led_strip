//
// Created by kok on 04.09.24.
//

#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_wifi.h"

#define WIFI_APP_STA_CHANNEL          1
#define WIFI_APP_STA_MAX_RETRIES      3
#define WIFI_APP_STA_MAX_AP_RECORDS   20

#define WIFI_APP_AP_SSID              "ESP32 AP"
#define WIFI_APP_AP_PASSWORD          "test1234"
#define WIFI_APP_AP_SSID_LENGTH        32
#define WIFI_APP_AP_CHANNEL            1
#define WIFI_APP_AP_MAX_CONNECTIONS    3
#define WIFI_APP_AP_DEFAULT_GATEWAY    "192.168.0.1"
#define WIFI_APP_AP_IP_ADDRESS         "192.168.0.1"
#define WIFI_APP_AP_NETMASK            "255.255.255.0"
#define WIFI_APP_AP_BANDWIDTH          WIFI_BW_HT20

typedef enum {
    WIFI_APP_MSG_CONNECT,
    WIFI_APP_MSG_DISCONNECT,
} wifi_app_msg_e;

typedef struct {
    wifi_app_msg_e msgID;
    void *params;
} wifi_app_message_t;

typedef struct {
    char *ssid;
    char *password;
} wifi_app_sta_connection_config_t;

typedef struct {
    wifi_ap_record_t *records;
    uint8_t records_count;
} wifi_app_sta_scan_results_t;

/**
 * Initialize the WiFi Application
 */
void wifi_app_init(void);

/**
 * Send message to the WiFi Applicatiom message queue
 */
void wifi_app_send_message(wifi_app_msg_e msgID, void *pvParams);

/**
 * Get the current wifi's sta configuration
 */
wifi_config_t *wifi_app_get_sta_config(void);

/**
 * Scan for remote AP's SSIDs
 */
esp_err_t wifi_app_sta_scan(wifi_app_sta_scan_results_t *results);
#endif //WIFI_APP_H
