#pragma once

// 主机语法自检用的 esp_log.h 桩：只保留 app_controller.cpp 用到的 ESP_LOGI。

#include <cstdio>

#define ESP_LOGI(tag, format, ...) std::printf("[%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) std::printf("[%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) std::printf("[%s] " format "\n", tag, ##__VA_ARGS__)
