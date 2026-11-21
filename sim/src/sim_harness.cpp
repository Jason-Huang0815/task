#include "sim_harness.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace sim {
namespace {

void flushCb(lv_display_t* disp, const lv_area_t* /*area*/, uint8_t* /*px*/) {
    // 全屏刷新模式：LVGL 直接画进 m_buffer，这里只需要“报告完成”。
    lv_display_flush_ready(disp);
}

void collectLabels(lv_obj_t* obj, std::string& out) {
    if (obj == nullptr) {
        return;
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        const char* text = lv_label_get_text(obj);
        if (text != nullptr && text[0] != '\0') {
            out += text;
            out += '\n';
        }
    }
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) {
        collectLabels(lv_obj_get_child(obj, i), out);
    }
}

bool subtreeHasText(lv_obj_t* obj, const char* needle) {
    if (obj == nullptr) {
        return false;
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        const char* text = lv_label_get_text(obj);
        if (text != nullptr && std::strstr(text, needle) != nullptr) {
            return true;
        }
    }
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) {
        if (subtreeHasText(lv_obj_get_child(obj, i), needle)) {
            return true;
        }
    }
    return false;
}

/// 自底向上找：最深的、可点击的、子树里含目标文字的普通对象。
lv_obj_t* findClickable(lv_obj_t* obj, const char* needle) {
    if (obj == nullptr) {
        return nullptr;
    }
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) {
        if (lv_obj_t* found = findClickable(lv_obj_get_child(obj, i), needle)) {
            return found;
        }
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        return nullptr;
    }
    if (!lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        return nullptr;
    }
    return subtreeHasText(obj, needle) ? obj : nullptr;
}

}  // namespace

std::string outputDir() {
    const char* env = std::getenv("SIM_OUT_DIR");
    return (env != nullptr && env[0] != '\0') ? std::string(env) : std::string("out");
}

bool ensureDir(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return !ec;
}

void Checks::expect(bool ok, const std::string& what) {
    ++m_total;
    if (!ok) {
        ++m_failed;
    }
    m_items.push_back({ok, what});
}

void Checks::dump(const char* title) const {
    std::printf("\n=== %s ===\n", title);
    for (const Item& item : m_items) {
        std::printf("  [%s] %s\n", item.ok ? "PASS" : "FAIL", item.what.c_str());
    }
    std::printf("--- %d checks, %d failed ---\n", m_total, m_failed);
}

Fixture::Fixture() : Fixture(nullptr) {}

Fixture::Fixture(lv_display_t* external)
    : m_settings(), m_bus(), m_steps(m_bus, m_settings), m_sedentary(m_bus, m_settings),
      m_bridge(m_bus, m_steps, m_sedentary, m_settings) {
    if (external != nullptr) {
        // 窗口模式：display 由 lv_windows_create_display 创建，刷新交给 lv_timer_handler
        m_display = external;
        m_ownsDisplay = false;
    } else {
        m_display = lv_display_create(kWidth, kHeight);
        if (m_display == nullptr) {
            std::printf("[sim] lv_display_create 失败\n");
            return;
        }
        m_buffer = static_cast<uint8_t*>(std::malloc(static_cast<size_t>(kWidth) * kHeight * 4u));
        if (m_buffer == nullptr) {
            std::printf("[sim] 帧缓冲分配失败\n");
            return;
        }
        std::memset(m_buffer, 0, static_cast<size_t>(kWidth) * kHeight * 4u);

        lv_display_set_color_format(m_display, LV_COLOR_FORMAT_ARGB8888);
        lv_display_set_buffers(m_display, m_buffer, nullptr,
                               static_cast<uint32_t>(kWidth) * kHeight * 4u,
                               LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_flush_cb(m_display, flushCb);
    }

    // 单线程：加锁是空操作。注意这两个对象必须活到 Fixture 析构，UiConfig 里存的是指针。
    m_backlightPort.setPct = [this](int pct) {
        m_backlight.lastPct = pct;
        ++m_backlight.pctCalls;
    };
    m_backlightPort.setPower = [this](bool on) {
        m_backlight.lastPower = on;
        ++m_backlight.powerCalls;
    };
    m_backlight.lastPct = m_settings.brightness;
    m_backlight.lastPower = true;

    m_ok = m_bridge.begin(lv_screen_active(), m_port, m_backlightPort);
    // 真机上 AppController 订阅 EventBus 后把事件转给界面；仿真里由夹具代劳。
    // EventBus 按类型订阅，没有通配，所以这里把界面关心的事件都挂上。
    const hb::EventType forwarded[] = {
        hb::EventType::KeyShort,     hb::EventType::KeyLong,   hb::EventType::KeyVeryLong,
        hb::EventType::Alert,        hb::EventType::UserStandUp, hb::EventType::Snooze,
        hb::EventType::AlertTimeout, hb::EventType::WearChanged, hb::EventType::Step,
        hb::EventType::MinuteTick,
    };
    for (const hb::EventType type : forwarded) {
        m_subIds.push_back(m_bus.subscribe(type, [this](const hb::Event& event) {
            m_bridge.onEvent(event);
        }));
    }
    pump();
}

Fixture::~Fixture() {
    for (const int id : m_subIds) {
        m_bus.unsubscribe(id);
    }
    m_subIds.clear();
    std::free(m_buffer);
    m_buffer = nullptr;
}

void Fixture::pump() {
    m_bridge.tick(m_clock.nowMs());
    if (m_hasOverride) {
        // UiBridge::tick 内部会用自己的 buildModel() 覆盖模型，所以注入要放在它之后，
        // 再驱动一次 UiService::tick（时间 +1 s 保证跨过刷新阈值），页面才会重画。
        m_bridge.ui().setModel(m_override);
        m_bridge.ui().tick(m_clock.nowMs() + 1000u);
    }
}

void Fixture::step() {
    pump();
    if (m_ownsDisplay) {
        lv_refr_now(m_display);  // 离屏模式：每拍重新渲染一帧，截图才能看到最新内容
    }
}

void Fixture::advance(uint32_t ms) {
    m_clock.advance(ms);
    step();
}

void Fixture::publish(hb::EventType type, int32_t value, hb::AlertKind alert) {
    hb::Event event{};
    event.type = type;
    event.value = value;
    event.alert = alert;
    m_bus.publish(event);
}

void Fixture::clearModelOverride() {
    m_hasOverride = false;
}

void Fixture::setModel(const hb::ui::Model& model) {
    m_override = model;
    m_hasOverride = true;
    m_bridge.ui().setModel(m_override);
}

std::string Fixture::labels() const {
    std::string out;
    collectLabels(lv_screen_active(), out);
    collectLabels(const_cast<Fixture*>(this)->m_bridge.ui().dialogLayer(), out);
    return out;
}

bool Fixture::hasText(const char* needle) const {
    const std::string all = labels();
    return all.find(needle) != std::string::npos;
}

bool Fixture::click(const char* text) {
    lv_obj_t* target = findClickable(lv_screen_active(), text);
    if (target == nullptr) {
        target = findClickable(m_bridge.ui().dialogLayer(), text);
    }
    if (target == nullptr) {
        return false;
    }
    lv_obj_send_event(target, LV_EVENT_CLICKED, nullptr);
    step();
    return true;
}

bool Fixture::capture(const std::string& name) {
    // 截图前把 LVGL 的动画时间推完（180 ms 淡入），否则会拍到"半张旧屏 + 半张新屏"。
    for (int i = 0; i < 20; ++i) {
        lv_tick_inc(16);
        lv_timer_handler();
    }
    // lv_tick_inc 驱动不出 LVGL 的屏幕淡入（离屏模式下动画进度按 handler 次数推进），
    // 截图前直接把"上一屏"收掉，否则会拍到两张屏叠加的半透明画面。
    lv_obj_t* prevScr = lv_display_get_screen_prev(m_display);
    if (prevScr != nullptr) {
        lv_obj_add_flag(prevScr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(lv_display_get_screen_active(m_display), LV_OPA_COVER, 0);
        lv_refr_now(m_display);
    }
    if (std::getenv("SIM_DEBUG") != nullptr) {
        std::printf("[debug] tick=%u\n", static_cast<unsigned>(lv_tick_get()));
        lv_obj_t* act = lv_display_get_screen_active(m_display);
        lv_obj_t* prev = lv_display_get_screen_prev(m_display);
        std::printf("[debug] %s act=%p prev=%p act_opa=%d prev_hidden=%d\n", name.c_str(),
                    static_cast<void*>(act), static_cast<void*>(prev),
                    static_cast<int>(lv_obj_get_style_opa(act, LV_PART_MAIN)),
                    (prev != nullptr && lv_obj_has_flag(prev, LV_OBJ_FLAG_HIDDEN)) ? 1 : 0);
    }
    if (m_buffer == nullptr) {
        return false;
    }
    const std::string dir = outputDir();
    if (!ensureDir(dir)) {
        return false;
    }
    lv_refr_now(m_display);
    const std::string path = dir + "/" + name + ".png";
    return savePng(path.c_str(), m_buffer, kWidth, kHeight);
}

}  // namespace sim
