idf_component_register(
    SRCS "src/mpu6050.cpp" "src/max30102.cpp" "src/tmp117.cpp"
    INCLUDE_DIRS "include"
    REQUIRES hb_common hb_hal driver esp_timer
)
