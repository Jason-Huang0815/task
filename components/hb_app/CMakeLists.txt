idf_component_register(
    SRCS
        "src/app_controller.cpp"
        "src/ui_bridge.cpp"
        "src/screens/watch_face.cpp"
        "src/screens/sport_screen.cpp"
        "src/screens/vital_screen.cpp"
        "src/screens/temp_screen.cpp"
        "src/screens/sedentary_screen.cpp"
        "src/screens/settings_screen.cpp"
        "src/screens/alert_overlay.cpp"
    INCLUDE_DIRS "include"
    REQUIRES hb_common hb_service hb_driver hb_ui lvgl
)
