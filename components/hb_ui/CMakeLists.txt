# 字库是可选产物：tools/gen_font_subset.ps1 跑过之后 src/fonts/ 才有 .c 文件。
# 没有字库时保持“中文字形不编进来”，固件仍然能编译；有字库时自动加入并打开
# HB_UI_HAS_CN_FONT，ui/fonts.h 里的 hb_font_cn_* 才会被引用到。
file(GLOB HB_FONT_SRC "src/fonts/*.c")

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
        ${HB_FONT_SRC}
    INCLUDE_DIRS "include"
    REQUIRES hb_common lvgl
)

if(HB_FONT_SRC)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC HB_UI_HAS_CN_FONT=1)
endif()
