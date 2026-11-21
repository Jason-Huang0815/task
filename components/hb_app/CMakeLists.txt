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

# Fonts are an optional artifact of hb_ui: enable Chinese glyphs only when
# components/hb_ui/src/fonts/*.c exists. Checked here as well so the app layer
# does not depend on PUBLIC definitions being propagated across components.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../hb_ui/src/fonts/hb_font_cn_16.c")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC HB_UI_HAS_CN_FONT=1)
endif()
