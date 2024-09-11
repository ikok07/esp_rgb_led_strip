//
// Created by kok on 08.09.24.
//

#ifndef MQTT_APP_H
#define MQTT_APP_H

#define MQTT_APP_BROKER_HOST           "mqtt-broker.lan"
#define MQTT_APP_BROKER_PORT           1883
#define MQTT_APP_BROKER_MAX_MSG_SIZE   1024

#define MQTT_APP_SUBSCRIBE_TOPIC       "home/controllers/led/receive"
#define MQTT_APP_PUBLISH_TOPIC         "home/controllers/led/send"
#define MQTT_APP_LAST_WILL_MSG         "{\"tag\": \"led_strip_diconnect\"}"
#define MQTT_APP_QOS                   1

#define MQTT_APP_CLIENT_ID             "esp_rgb_led_strip"
#define MQTT_APP_USERNAME              "led-strip-client"
#define MQTT_APP_PASSWORD              "NQXkhiDZtd7rZGWQNmV9"

#define MQTT_APP_TAG_LED_STRIP         "led_strip"

/**
* Start the MQTT Communication Application
*/
void mqtt_app_init(void);

#endif //MQTT_APP_H
