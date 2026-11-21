#pragma once

// 仿真垫片：hb_app 里少数文件会 include "esp_log.h"（真机日志）。
// 仿真不编译 app_controller.cpp，这里只保证“即使编到也不会因为缺头文件而失败”。
// 故意写成丢弃参数的形式：仿真输出走 std::printf，避免和 LVGL 日志交织。

#define ESP_LOGE(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGD(tag, ...) ((void)(tag))
#define ESP_LOGV(tag, ...) ((void)(tag))
