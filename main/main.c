#include "app_nvs/app_nvs.h"
#include "app_spiffs/app_spiffs.h"
#include "wifi_app/wifi_app.h"
#include "http_server/http_server.h"
#include "rmt/rmt_app.h"
#include "object_sensor/object_sensor.h"
#include "mode_switcher/mode_switcher.h"
#include "mqtt_app/mqtt_app.h"

void app_main() {

    // Initialize NVS
    app_nvs_init();

    // Initialize SPIFFS
    // app_spiffs_init();

    // Start WiFi and HTTP server
    wifi_app_init();
    http_server_init();

    // Start MQTT application
    // TODO: Start MQTT after Wifi connection is established
    mqtt_app_init();

    rmt_app_start();
    object_sensor_init();
    mode_switcher_init();
}
