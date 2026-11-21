#pragma once

// 仿真垫片：固件用 esp_timer_get_time() 取单调微秒。
// PC 上用 steady_clock 提供同样的语义（只增不减，与系统时间无关）。

#include <chrono>
#include <cstdint>

inline int64_t esp_timer_get_time(void) {
    using Clock = std::chrono::steady_clock;
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch())
            .count());
}
