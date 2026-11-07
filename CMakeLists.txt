# ============================================================================
#  task —— 基于 ESP32-S3 的智能健康手环
#
#  两种构建模式：
#    1) ESP-IDF 固件（默认）：idf.py -B build/idf build
#    2) 主机侧单元测试      ：cmake --preset host-debug
#
#  主机模式只编译 hb_common + hb_service + tests/host，
#  不依赖 ESP-IDF、不需要开发板，用于算法与协议的快速回归。
# ============================================================================
cmake_minimum_required(VERSION 3.16)

option(TASK_HOST "Build host-side unit tests instead of ESP-IDF firmware" OFF)

# 两种模式都固定 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(TASK_HOST)
    project(task_host LANGUAGES CXX)

    set(CMAKE_CXX_EXTENSIONS OFF)

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()

    if(MSVC)
        add_compile_options(/W4 /permissive-)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()

    enable_testing()
    add_subdirectory(tests/host)
else()
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(task)
endif()
