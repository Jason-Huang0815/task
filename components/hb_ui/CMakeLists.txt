# 目前只有头文件（主题常量）。P4 接入 LVGL 后在此追加 SRCS 与 REQUIRES lvgl。
idf_component_register(
    INCLUDE_DIRS "include"
)
