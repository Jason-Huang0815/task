cmake_minimum_required(VERSION 3.20)
project(hb_ui_sim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(LVGL_ROOT "${REPO_ROOT}/managed_components/lvgl__lvgl" CACHE PATH "LVGL 9.2.x 源码目录")
if(NOT EXISTS "${LVGL_ROOT}/lvgl.h")
    message(FATAL_ERROR
        "找不到 LVGL 源码：${LVGL_ROOT}\n"
        "  办法 A：先在仓库根跑一次 idf.py build，组件管理器会拉下 lvgl__lvgl\n"
        "  办法 B：git clone --depth 1 --branch v9.2.2 https://github.com/lvgl/lvgl sim/third_party/lvgl\n"
        "          再 cmake -DLVGL_ROOT=sim/third_party/lvgl ...\n")
endif()

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE FILEPATH "仿真用 lv_conf.h")

add_subdirectory("${LVGL_ROOT}" "${CMAKE_BINARY_DIR}/lvgl")
if(MSVC)
    target_compile_options(lvgl PRIVATE /W0 /utf-8)
endif()

set(HB_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/shim"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${REPO_ROOT}/components/hb_common/include"
    "${REPO_ROOT}/components/hb_service/include"
    "${REPO_ROOT}/components/hb_driver/include"
    "${REPO_ROOT}/components/hb_ui/include"
    "${REPO_ROOT}/components/hb_app/include"
)

set(HB_UI_SRC
    "${REPO_ROOT}/components/hb_ui/src/model.cpp"
    "${REPO_ROOT}/components/hb_ui/src/nav_model.cpp"
    "${REPO_ROOT}/components/hb_ui/src/event_queue.cpp"
    "${REPO_ROOT}/components/hb_ui/src/widgets.cpp"
    "${REPO_ROOT}/components/hb_ui/src/dialog.cpp"
    "${REPO_ROOT}/components/hb_ui/src/screen_manager.cpp"
    "${REPO_ROOT}/components/hb_ui/src/ui_service.cpp"
    "${REPO_ROOT}/components/hb_ui/src/lvgl_compat.cpp"
)

set(HB_APP_SRC
    "${REPO_ROOT}/components/hb_app/src/ui_bridge.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/watch_face.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/sport_screen.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/vital_screen.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/temp_screen.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/sedentary_screen.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/settings_screen.cpp"
    "${REPO_ROOT}/components/hb_app/src/screens/alert_overlay.cpp"
)

set(HB_SERVICE_SRC
    "${REPO_ROOT}/components/hb_service/src/event_bus.cpp"
    "${REPO_ROOT}/components/hb_service/src/step_service.cpp"
    "${REPO_ROOT}/components/hb_service/src/sedentary_service.cpp"
)

# 字体：生成过就编进去；没生成也能跑，只是中文回落默认字体（场景测试会报出来）。
set(HB_FONT_DIR "${REPO_ROOT}/components/hb_ui/src/fonts")
file(GLOB HB_FONT_SRC "${HB_FONT_DIR}/*.c")
list(LENGTH HB_FONT_SRC HB_FONT_COUNT)
if(HB_FONT_COUNT GREATER 0)
    message(STATUS "字体源文件 ${HB_FONT_COUNT} 个：${HB_FONT_DIR}")
    list(APPEND HB_UI_SRC ${HB_FONT_SRC})
    set(HB_FONT_DEFINE "HB_UI_HAS_CN_FONT=1")
else()
    message(WARNING "未找到中文字体（${HB_FONT_DIR}），中文会显示方框；先跑 tools/gen_font_subset.ps1")
    set(HB_FONT_DEFINE "HB_UI_HAS_CN_FONT=0")
endif()

set(SIM_CORE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sim_harness.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sim_capture.cpp"
)

function(sim_add_target target main_src)
    add_executable(${target} ${main_src} ${SIM_CORE_SRC} ${HB_UI_SRC} ${HB_APP_SRC} ${HB_SERVICE_SRC})
    target_include_directories(${target} PRIVATE ${HB_INCLUDES})
    target_compile_definitions(${target} PRIVATE ${HB_FONT_DEFINE})
    target_link_libraries(${target} PRIVATE lvgl)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4 /permissive-)
        target_link_libraries(${target} PRIVATE user32 gdi32 dwmapi)
    endif()
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()

# 交互窗口：真实鼠标点按钮，键盘注入硬件事件
sim_add_target(ui_sim "${CMAKE_CURRENT_SOURCE_DIR}/src/sim_app.cpp")

# 场景回归：不建窗口，渲染到内存 + 截图 + 断言
sim_add_target(ui_sim_tests "${CMAKE_CURRENT_SOURCE_DIR}/src/sim_scenarios.cpp")
