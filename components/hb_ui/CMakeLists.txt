idf_component_register(
    SRCS
        "src/model.cpp"
        "src/nav_model.cpp"
        "src/event_queue.cpp"
        "src/widgets.cpp"
        "src/dialog.cpp"
        "src/screen_manager.cpp"
        "src/ui_service.cpp"
        "src/lvgl_compat.cpp"
    INCLUDE_DIRS "include"
    REQUIRES hb_common lvgl
)
