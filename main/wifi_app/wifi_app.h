//
// Created by kok on 04.09.24.
//

#ifndef WIFI_APP_H
#define WIFI_APP_H

#define WIFI_APP_AP_SSID              "ESP32 AP"
#define WIFI_APP_AP_PASSWORD          "test1234"
#define WIFI_APP_AP_SSID_LENGTH        32
#define WIFI_APP_AP_CHANNEL            1
#define WIFI_APP_AP_MAX_CONNECTIONS    3
#define WIFI_APP_AP_DEFAULT_GATEWAY    "192.168.0.1"
#define WIFI_APP_AP_IP_ADDRESS         "192.168.0.1"
#define WIFI_APP_AP_NETMASK            "255.255.255.0"

typedef enum {
    WIFI_APP_MSG_CONNECT,
    WIFI_APP_MSG_DISCONNECT
} wifi_app_msg_e;

typedef struct {
    wifi_app_msg_e msgID;
} wifi_app_message_t;

/**
 * Initialize the WiFi Application
 */
void wifi_app_init(void);

#endif //WIFI_APP_H
