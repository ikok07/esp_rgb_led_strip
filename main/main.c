#include "esp_log.h"

#include "app_nvs/app_nvs.h"
#include "app_spiffs/app_spiffs.h"
#include "wifi_app/wifi_app.h"
#include "http_server/http_server.h"
#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"
#include "mqtt_app/mqtt_app.h"

/**
 * Callback function which is called upon establishing a WiFi connection
 */
void wifi_app_connected_cb(void) {
    // Start MQTT application
    // mqtt_app_init();
}

void app_main() {

    // Initialize NVS
    app_nvs_init();

    // Initialize SPIFFS
    app_spiffs_init();

    // Start WiFi and HTTP server
    wifi_app_cb_set(wifi_app_connected_cb);
    wifi_app_init();
    http_server_init();

    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
