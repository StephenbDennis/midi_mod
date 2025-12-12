file(GLOB SRC_FILES "*.cpp")
idf_component_register(SRCS ${SRC_FILES}
                    PRIV_REQUIRES esp_adc esp_timer fatfs wear_levelling esp_partition
                    INCLUDE_DIRS ".")
