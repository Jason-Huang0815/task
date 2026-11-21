#pragma once

#include <cstdint>
#include <functional>

// 平台端口。hb_ui 不 include 任何 esp_* / lvgl port 头文件，
// 临界区与背光都由宿主（main 或仿真垫片）从这里注入。

namespace hb::ui {

/// LVGL 临界区与时钟。lock/unlock 通常绑定 lvgl_port_lock(0)/unlock。
struct Port {
    std::function<void()> lock;
    std::function<void()> unlock;

    bool valid() const { return static_cast<bool>(lock) && static_cast<bool>(unlock); }
};

/// 背光控制。BL 引脚经 MP3302 升压，LEDC PWM 输出 0–100%，
/// 0 即熄屏（只关背光，LVGL 继续跑）。
struct BacklightPort {
    std::function<void(int)> setPct;     // 0–100
    std::function<void(bool)> setPower;  // 物理开/关（可选）

    bool valid() const { return static_cast<bool>(setPct); }
};

}  // namespace hb::ui
