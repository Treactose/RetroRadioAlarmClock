idf_component_register(
    SRCS
        "main.cpp"
        "buttons/Buttons.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        esp_common
        freertos
        log
)

