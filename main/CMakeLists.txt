idf_component_register(SRCS "main.c" "onewire_bus.c" "ds18b20.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp-zigbee-lib
                    PRIV_REQUIRES esp_timer)
