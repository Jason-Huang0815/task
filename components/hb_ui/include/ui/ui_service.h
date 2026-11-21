#pragma once

#include <cstdint>
#include <functional>
#include <mutex>

#include "common/events.h"
#include "lvgl.h"
#include "ui/event_queue.h"
#include "ui/model.h"
#include "ui/nav_model.h"
#include "ui/port.h"
#include "ui/screen_manager.h"

// 界面门面：持有页面、导航、事件队列、状态栏/页点/弹层。
// 页面是独立 screen 对象，因此状态栏与弹层在每次切屏后挂到当前屏上。

namespace hb::ui {

struct UiConfig {
    const Port* port = nullptr;
    const BacklightPort* backlight = nullptr;
    int brightnessDefaultPct = 80;
};

/// 界面层与业务层的唯一交界：所有按钮动作都从这里出去，不直接调 service。
struct UiCallbacks {
    std::function<void()> standUp;   // "我起来了"/弹窗"已起身"
    std::function<void()> snooze;    // "稍后 10 分钟"
    std::function<void()> syncNow;   // 设置页"立即同步"
    std::function<void(int)> setBrightness;
    std::function<void(int)> setThreshold;
    std::function<void(int)> setVibration;
    std::function<void(bool)> setWristRaise;
    std::function<void(hb::AlertKind, int)> showAlert;  // (类型, 已坐分钟)
    std::function<void(int)> updateAlertCountdown;      // 剩余秒数，每秒一次
    std::function<void()> hideAlert;
    /// 跌倒弹窗"立即求助"：按 SOS 处理。其余页面不使用这个动作。
    std::function<void()> fallSos;
};

class UiService {
public:
    /// statusHost：启动时的宿主屏；页面装载后状态栏/页点/弹层会改挂到当前屏。
    bool begin(lv_obj_t* statusHost, const UiConfig& config);

    /// 注册页面（会触发 Screen::createScreen()）。
    bool addScreen(Screen* screen);

    /// 显示首页并开始 tick 循环。
    void start();

    /// ui_task 里循环调用，nowMs 用 lv_tick_get()。
    void tick(uint32_t nowMs);

    void setModel(const Model& model);
    Model snapshot() const;

    void postEvent(const hb::Event& event);
    void setCallbacks(const UiCallbacks& callbacks);

    NavModel& nav() { return m_nav; }
    uint32_t droppedEvents() const { return m_queue.dropped(); }
    lv_obj_t* dialogLayer() const { return m_dialogLayer; }

    /// 关闭业务弹窗并解锁导航；页面按钮、弹窗按钮、倒计时结束都走这里。
    void dismissAlert();

private:
    void handleEvent(const hb::Event& event, uint32_t nowMs);
    void openAlert(hb::AlertKind kind, int value, uint32_t nowMs);
    void showPage(PageId id, bool animate);
    void attachChrome(lv_obj_t* host);
    void detachChrome();
    void updateChrome(const Model& model);
    void syncDots(int index);
    void applyBrightness(int pct);

    UiConfig m_config;
    UiCallbacks m_callbacks;
    ScreenManager m_screens;
    NavModel m_nav;
    EventQueue m_queue;

    mutable std::mutex m_modelMutex;
    Model m_model;

    // 状态栏 / 页点 / 弹层，随活动屏重建
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_dotsBar = nullptr;
    lv_obj_t* m_statusTime = nullptr;
    lv_obj_t* m_statusState = nullptr;
    lv_obj_t* m_statusBattery = nullptr;
    lv_obj_t* m_dot[kPageCount] = {nullptr};
    lv_obj_t* m_dialogLayer = nullptr;

    bool m_started = false;
    bool m_dialogVisible = false;
    bool m_dirty = true;  // 首帧或刚切屏时立刻刷新一次
    int m_brightness = -1;
    hb::AlertKind m_alertKind = hb::AlertKind::None;
    uint32_t m_alertDeadlineMs = 0;
    int m_lastCountdown = -1;
    uint32_t m_lastRefreshMs = 0;
};

}  // namespace hb::ui
