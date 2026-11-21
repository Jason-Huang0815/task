#pragma once

// 仿真夹具：把 hb_ui + hb_app/screens 架在 LVGL 9.2 上，不接 ESP-IDF、不接开发板。
// 交互窗口（sim_app.cpp）与场景测试（sim_scenarios.cpp）共用这一套。

#include <cstdint>
#include <string>
#include <vector>

#include "app/ui_bridge.h"
#include "common/events.h"
#include "common/settings.h"
#include "lvgl.h"
#include "service/event_bus.h"
#include "service/sedentary_service.h"
#include "service/step_service.h"
#include "ui/model.h"
#include "ui/nav_model.h"
#include "ui/port.h"

namespace sim {

constexpr int kWidth = 800;
constexpr int kHeight = 480;

/// 虚拟时钟：测试里时间必须可控，否则 60 s 倒计时没法测。
class Clock {
public:
    uint32_t nowMs() const { return m_ms; }
    void advance(uint32_t ms) { m_ms += ms; }
    void set(uint32_t ms) { m_ms = ms; }

private:
    uint32_t m_ms = 0;
};

/// 背光探针：记录 setPct / setPower 的调用，用来断言“长按真的关了背光”。
struct BacklightSpy {
    int lastPct = 100;
    bool lastPower = true;
    int pctCalls = 0;
    int powerCalls = 0;

    void reset() {
        lastPct = 100;
        lastPower = true;
        pctCalls = 0;
        powerCalls = 0;
    }
};

/// 断言收集器：失败也继续跑，最后统一报告，一次看清所有问题。
class Checks {
public:
    void expect(bool ok, const std::string& what);
    int failed() const { return m_failed; }
    int total() const { return m_total; }
    void dump(const char* title) const;

private:
    struct Item {
        bool ok;
        std::string what;
    };
    std::vector<Item> m_items;
    int m_failed = 0;
    int m_total = 0;
};

/// 一套跑起来的界面：真实 UiBridge + 真实 service + 假传感器/假时钟。
class Fixture {
public:
    Fixture();
    /// 复用外部 display（例如 lv_windows_create_display 开的窗口）。
    explicit Fixture(lv_display_t* display);
    ~Fixture();

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    bool ok() const { return m_ok; }

    /// 推进一拍：UiBridge::tick(nowMs)。
    void step();
    /// 时钟前进 ms 毫秒，然后推进一拍。
    void advance(uint32_t ms);

    /// 注入一条事件（按键/告警等），随后需 step() 才会被处理。
    void publish(hb::EventType type, int32_t value = 0,
                 hb::AlertKind alert = hb::AlertKind::None);

    /// 直接注入一帧数据（测试渲染用；会绕过 service，下一次 step 不会覆盖）。
    void setModel(const hb::ui::Model& model);
    /// 撤销注入，回到“由 service 驱动”的正常模式。
    void clearModelOverride();

    /// 当前屏幕 + 弹层里所有 lv_label 的文字，用换行连接。
    std::string labels() const;
    bool hasText(const char* needle) const;

    /// 找到“里面含有这段文字”的可点对象并发送 LV_EVENT_CLICKED。
    bool click(const char* text);

    /// 把当前帧渲染出来存成 PNG；成功返回 true。
    bool capture(const std::string& name);

    ::app::UiBridge& bridge() { return m_bridge; }
    hb::ui::UiService& ui() { return m_bridge.ui(); }
    hb::ui::NavModel& nav() { return m_bridge.ui().nav(); }
    hb::EventBus& bus() { return m_bus; }
    hb::StepService& steps() { return m_steps; }
    hb::SedentaryService& sedentary() { return m_sedentary; }
    hb::Settings& settings() { return m_settings; }
    BacklightSpy& backlight() { return m_backlight; }
    uint32_t nowMs() const { return m_clock.nowMs(); }
    lv_display_t* display() const { return m_display; }
    /// 离屏帧缓冲（ARGB8888，800×480×4 字节），供窗口直接贴图用。
    const uint8_t* pixels() const { return m_buffer; }

private:
    void pump();

    Clock m_clock;
    BacklightSpy m_backlight;
    hb::Settings m_settings;
    hb::EventBus m_bus;
    hb::StepService m_steps;
    hb::SedentaryService m_sedentary;
    ::app::UiBridge m_bridge;

    lv_display_t* m_display = nullptr;
    uint8_t* m_buffer = nullptr;
    bool m_ownsDisplay = true;
    hb::ui::Port m_port;
    hb::ui::BacklightPort m_backlightPort;
    bool m_hasOverride = false;
    hb::ui::Model m_override;
    std::vector<int> m_subIds;
    bool m_ok = false;
};

/// 截屏输出目录（相对运行目录，默认 sim/out，可用 SIM_OUT_DIR 覆盖）。
std::string outputDir();
bool ensureDir(const std::string& dir);

/// 把 LVGL 的 ARGB8888 帧存成 PNG（自写编码器，不依赖 libpng/PIL）。
bool savePng(const char* path, const uint8_t* argb, int width, int height);

}  // namespace sim
