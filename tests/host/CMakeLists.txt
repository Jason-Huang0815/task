set(TASK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# hb_common：纯头文件（version.cpp 主机侧不需要）
add_library(hb_common INTERFACE)
target_include_directories(hb_common INTERFACE "${TASK_ROOT}/components/hb_common/include")

# 只编译与硬件无关的服务层，这是主机测试能跑起来的关键
add_library(hb_service STATIC
    "${TASK_ROOT}/components/hb_service/src/event_bus.cpp"
    "${TASK_ROOT}/components/hb_service/src/step_service.cpp"
    "${TASK_ROOT}/components/hb_service/src/sedentary_service.cpp"
)
target_include_directories(hb_service PUBLIC "${TASK_ROOT}/components/hb_service/include")
target_link_libraries(hb_service PUBLIC hb_common)
target_compile_features(hb_service PUBLIC cxx_std_17)

add_executable(task_tests
    test_main.cpp
    test_event_bus.cpp
    test_step.cpp
    test_sedentary.cpp
)
target_link_libraries(task_tests PRIVATE hb_service)
target_include_directories(task_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(task_tests PRIVATE cxx_std_17)

add_test(NAME task_unit COMMAND task_tests)

# ---------------------------------------------------------------------------
# hb_ui 纯逻辑（不碰 LVGL）：格式化、导航、事件队列
# 这三个源文件是页面 refresh() 里唯一允许出现的"算法"，所以在主机上测它们
# 就等价于测了六个页面的显示逻辑。
# ---------------------------------------------------------------------------
add_library(hb_ui_logic STATIC
    "${TASK_ROOT}/components/hb_ui/src/model.cpp"
    "${TASK_ROOT}/components/hb_ui/src/nav_model.cpp"
    "${TASK_ROOT}/components/hb_ui/src/event_queue.cpp"
)
target_include_directories(hb_ui_logic PUBLIC
    "${TASK_ROOT}/components/hb_ui/include"
    "${TASK_ROOT}/components/hb_common/include"
)
target_link_libraries(hb_ui_logic PUBLIC hb_common)
target_compile_features(hb_ui_logic PUBLIC cxx_std_17)

add_executable(ui_logic_tests
    ui_test_main.cpp
    tst_ui_logic.cpp
)
target_link_libraries(ui_logic_tests PRIVATE hb_ui_logic)
target_include_directories(ui_logic_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(ui_logic_tests PRIVATE cxx_std_17)

if(MSVC)
    # 源码里有中文注释与中文字符串，必须显式指定 UTF-8。
    # 不加 /utf-8 时 MSVC 按系统 GBK 读 UTF-8 源码，句子末尾的"。"会把换行一起吃掉，
    # 下一行的 struct/函数声明就被并进注释里，报出一堆 "缺少 ;" —— 服务层同样中招。
    foreach(target hb_service task_tests hb_ui_logic ui_logic_tests)
        target_compile_options(${target} PRIVATE /utf-8 /W4)
    endforeach()
endif()

add_test(NAME ui_logic COMMAND ui_logic_tests)
