file(GLOB_RECURSE SRC_FILES *.*)

idf_component_register(SRCS main.c ${SRC_FILES}
                        INCLUDE_DIRS ".")

spiffs_create_partition_image(storage ../spiffs_image/esp_wifi_connection_panel FLASH_IN_PROJECT)