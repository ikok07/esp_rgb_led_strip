//
// Created by kok on 05.09.24.
//

#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "sys/param.h"

#include "tasks_common.h"
#include "wifi_app/wifi_app.h"
#include "http_server.h"

#include <cJSON.h>

static const char TAG[] = "http_server";

static httpd_handle_t http_server_handle = NULL;
static QueueHandle_t http_server_monitor_queue = NULL;
static EventGroupHandle_t http_server_event_group = NULL;

static uint32_t HTTP_SERVER_WIFI_CONNECTION_UPDATED         = BIT0;

static http_server_wifi_connect_status_e g_http_server_wifi_connect_status = NONE;

// --------- MONITOR TASK --------- //

/**
 * The main HTTP server's monitor task
 * @param pvParams optional task parameters
 */
static void http_server_monitor_task(void *pvParams) {
    http_server_message_t msg;
    while(1) {
        if (xQueueReceive(http_server_monitor_queue, &msg, portMAX_DELAY)) {
            switch (msg.msgID) {
                case HTTP_SERVER_MSG_WIFI_CONNECTING:
                    ESP_LOGI(TAG, "HTTP_SERVER_MSG_WIFI_CONNECTING");
                    g_http_server_wifi_connect_status = HTTP_SERVER_WIFI_STATUS_CONNECTING;
                    break;
                case HTTP_SERVER_MSG_WIFI_CONNECTED:
                    ESP_LOGI(TAG, "HTTP_SERVER_MSG_WIFI_CONNECTED");
                    g_http_server_wifi_connect_status = HTTP_SERVER_WIFI_STATUS_CONNECTED;
                    break;
                case HTTP_SERVER_MSG_WIFI_DISCONNECTED:
                    ESP_LOGI(TAG, "HTTP_SERVER_MSG_WIFI_DISCONNECTED");
                    g_http_server_wifi_connect_status = HTTP_SERVER_WIFI_STATUS_DISCONNECTED;
                    break;
            }
            xEventGroupSetBits(http_server_event_group, HTTP_SERVER_WIFI_CONNECTION_UPDATED);
        }
    }
}

// --------- URI HANDLERS --------- //

/**
 * Set the required CORS headers required by the browsers
 */
static void set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

static esp_err_t get_available_remote_ap_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Available remote AP's requested");
    set_cors_headers(req);
    wifi_app_sta_scan_results_t results;
    const esp_err_t err = wifi_app_sta_scan(&results);
    httpd_resp_set_type(req, "application/json");

    char responseJSON[1024];
    int offset = 0;

    if (err == ESP_OK) {
        offset += snprintf(
            responseJSON + offset,
            sizeof(responseJSON) - offset,
            "{\"status\": \"success\", \"available_networks\": ["
        );
        for (int i = 0; i < results.records_count; i++) {
            offset += snprintf(
                responseJSON + offset,
                sizeof(responseJSON) - offset,
                "{\"ssid\":\"%s\", \"signal_strength\": %d}%s",
                results.records[i].ssid, results.records[i].rssi, (i < results.records_count - 1) ? "," : ""
            );
        }
        offset += snprintf(responseJSON + offset, sizeof(responseJSON) - offset, "]}");
    } else snprintf(responseJSON, sizeof(responseJSON), "{\"status\": \"fail\", \"available_networks\": []");

    httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_connection_status_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WIFI conenction status requested");
    wifi_config_t *config = wifi_app_get_sta_config();
    wifi_ap_record_t ap_info;

    set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    char responseJSON[200];
    if (g_http_server_wifi_connect_status == HTTP_SERVER_WIFI_STATUS_CONNECTED && esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        snprintf(
            responseJSON,
            sizeof(responseJSON),
            "{\"status\": \"success\", \"wifi_status\": %d, \"ssid\": \"%s\", \"signal_strength\": %d}",
            g_http_server_wifi_connect_status, config->sta.ssid, ap_info.rssi
        );
    } else snprintf(responseJSON, sizeof(responseJSON), "{\"status\": \"success\", \"wifi_status\": %d}", g_http_server_wifi_connect_status);

    httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t wifi_connect_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WIFI connect requested");
    set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");
    char body[300];
    const size_t body_size = MIN(req->content_len, sizeof(body));

    const int recv_body_len = httpd_req_recv(req, body, body_size);
    if (recv_body_len < 0) {
        if (recv_body_len == HTTPD_SOCK_ERR_TIMEOUT) ESP_LOGE(TAG, "Socket timeout");
        else ESP_LOGE(TAG, "HTTP POST request error: %d", recv_body_len);
        return ESP_FAIL;
    }
    body[recv_body_len] = '\0';

    const cJSON *json = cJSON_Parse(body);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "The provided body is not a valid JSON!");
        return ESP_FAIL;
    }

    const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    if (!cJSON_IsString(ssid) || (ssid->valuestring == NULL)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Please, provide a valid SSID!");
        return ESP_FAIL;
    }

    const cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (!cJSON_IsString(password) || (password->valuestring == NULL)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Please, provide a valid SSID!");
        return ESP_FAIL;
    }

    wifi_config_t *config = wifi_app_get_sta_config();
    memcpy(config->sta.ssid, ssid->valuestring, strlen(ssid->valuestring) + 1);
    memcpy(config->sta.password, password->valuestring, strlen(password->valuestring) + 1);
    ESP_LOGI(TAG, "%s", config->sta.ssid);
    ESP_LOGI(TAG, "%s", config->sta.password);
    wifi_app_send_message(WIFI_APP_MSG_CONNECT, NULL);

    char responseJSON[100];
    xEventGroupClearBits(http_server_event_group, HTTP_SERVER_WIFI_CONNECTION_UPDATED);

    xEventGroupWaitBits(http_server_event_group, HTTP_SERVER_WIFI_CONNECTION_UPDATED, pdTRUE, pdTRUE, pdMS_TO_TICKS(30000));
    if (g_http_server_wifi_connect_status == HTTP_SERVER_WIFI_STATUS_CONNECTED)
        snprintf(responseJSON, sizeof(responseJSON), "{\"status\": \"success\"}");
    else {
        snprintf(responseJSON, sizeof(responseJSON), "{\"status\": \"fail\"}");
        httpd_resp_set_status(req, HTTPD_500);
    }

    httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t wifi_disconnect_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WIFI disconnect requested");
    set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    char responseJSON[200];
    wifi_app_send_message(WIFI_APP_MSG_DISCONNECT, NULL);
    snprintf(responseJSON, sizeof(responseJSON), "{\"status\": \"success\"}");
    httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * Import the HTTP Server's URI handlers
 */
static void http_server_uri_handlers() {
    const httpd_uri_t get_available_remote_ap = {
        .uri = "/remote/ap",
        .method = HTTP_GET,
        .handler = get_available_remote_ap_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(http_server_handle, &get_available_remote_ap);

    const httpd_uri_t get_connection_status = {
        .uri = "/remote/ap/connection",
        .method = HTTP_GET,
        .handler = get_connection_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(http_server_handle, &get_connection_status);

    const httpd_uri_t wifi_connect = {
        .uri = "/remote/ap/connect",
        .method = HTTP_POST,
        .handler = wifi_connect_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(http_server_handle, &wifi_connect);

    const httpd_uri_t wifi_diconnect = {
        .uri = "/remote/ap/disconnect",
        .method = HTTP_POST,
        .handler = wifi_disconnect_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(http_server_handle, &wifi_diconnect);
}

// --------- INITIAL CONFIGURATION --------- //

void http_server_init() {
    ESP_LOGI(TAG, "Initializing HTTP server");

    // Configure monitor queue and event group
    http_server_monitor_queue = xQueueCreate(3, sizeof(http_server_message_t));
    http_server_event_group = xEventGroupCreate();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;
    config.core_id = HTTP_SERVER_TASK_CORE_ID;
    config.max_uri_handlers = HTTP_SERVER_MAX_URI_HANDLERS;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    // Start the HTTP server
    esp_err_t err = httpd_start(&http_server_handle, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return;
    }
    ESP_LOGI(TAG, "Successfully started HTTP server!");

    // Add URI handlers
    http_server_uri_handlers();
    ESP_LOGI(TAG, "HTTP URI handlers were successfully added!");

    xTaskCreatePinnedToCore(
        &http_server_monitor_task,
        "http_server_monitor_task",
        HTTP_SERVER_TASK_STACK_SIZE,
        NULL,
        HTTP_SERVER_TASK_PRIORITY,
        NULL,
        HTTP_SERVER_TASK_CORE_ID
    );
    ESP_LOGI(TAG, "HTTP monitor task successfully started!");
}

void http_server_send_message(http_server_msg_e msgID, void *pvParams) {
    http_server_message_t msg = {.msgID = msgID, .params = pvParams};
    xQueueSend(http_server_monitor_queue, &msg, portMAX_DELAY);
}
