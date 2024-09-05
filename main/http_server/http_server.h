//
// Created by kok on 05.09.24.
//

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define HTTP_SERVER_MAX_URI_HANDLERS          20

typedef enum {
  NONE = 0,
  HTTP_SERVER_WIFI_STATUS_CONNECTING,
  HTTP_SERVER_WIFI_STATUS_CONNECTED,
  HTTP_SERVER_WIFI_STATUS_DISCONNECTED
} http_server_wifi_connect_status_e;

typedef enum {
 HTTP_SERVER_MSG_WIFI_CONNECTING,
 HTTP_SERVER_MSG_WIFI_CONNECTED,
 HTTP_SERVER_MSG_WIFI_DISCONNECTED,
} http_server_msg_e;

typedef struct {
 http_server_msg_e msgID;
 void *params;
} http_server_message_t;

/**
 * Initialize the HTTP server
 */
void http_server_init();

/**
 * Send message to the HTTP server monitor queue
 */
void http_server_send_message(http_server_msg_e msgID, void *pvParams);

#endif //HTTP_SERVER_H
