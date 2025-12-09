file(GLOB SRC_FILES "*.cpp")
idf_component_register(SRCS ${SRC_FILES}
                    PRIV_REQUIRES esp_adc
                    PRIV_REQUIRES esp_timer
                    INCLUDE_DIRS ".")
