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
