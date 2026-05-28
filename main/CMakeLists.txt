idf_component_register(SRCS "main.c" "onewire_bus.c" "ds18b20.c" "temp_reporting.c" "zigbee_ota.c" "contact_sensor.c" "status_led.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp-zigbee-lib
                    PRIV_REQUIRES esp_timer esp_app_format app_update)

