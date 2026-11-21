#include "ui/ui_service.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/lvgl_compat.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// 界面门面实现：事件队列 -> 导航 -> 页面 -> 状态栏/页点/弹层。
// tick 全程持锁，保证与渲染任务（esp_lvgl_port）不并发碰 LVGL 对象。

namespace hb::ui {
namespace {

/// 简单的临界区守卫：Port 未注入时退化为不锁（主机仿真/单元测试）。
class ScopedLock {
public:
    explicit ScopedLock(const Port* port) : m_port(port) {
        if (m_port != nullptr && m_port->lock) {
            m_port->lock();
        }
    }
    ~ScopedLock() {
        if (m_port != nullptr && m_port->unlock) {
            m_port->unlock();
        }
    }
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    const Port* m_port;
};

}  // namespace

bool UiService::begin(lv_obj_t* statusHost, const UiConfig& config) {
    m_config = config;
    m_started = false;
    m_dialogVisible = false;
    m_dirty = true;
    m_nav.setLocked(false);
    m_nav.home();
    m_screens.begin();

    lv_obj_t* host = statusHost != nullptr ? statusHost : compat::activeScreen();
    attachChrome(host);
    if (m_statusBar == nullptr) {
        return false;
    }

    m_brightness = -1;  // 强制把默认亮度真正下发一次
    applyBrightness(config.brightnessDefaultPct);
    updateChrome(snapshot());
    syncDots(static_cast<int>(m_screens.active()));
    return true;
}

bool UiService::addScreen(Screen* screen) { return m_screens.add(screen); }

void UiService::start() {
    if (m_started) {
        return;
    }
    m_nav.home();
    showPage(PageId::Watch, false);
    m_started = true;
    m_dirty = true;
}

void UiService::tick(uint32_t nowMs) {
    // 1) 排空事件队列：键盘/告警/传感器事件都在这里进入界面层。
    ScopedLock guard(m_config.port);
    hb::Event event;
    while (m_queue.pop(event)) {
        handleEvent(event, nowMs);
    }

    const Model snap = snapshot();

    // 2) 背光：模型里的亮度是产品设置；用户长按关屏（0）时不被覆盖。
    if (m_brightness > 0) {
        applyBrightness(static_cast<int>(snap.brightness));
    }

    // 3) 弹窗倒计时：每秒回调一次，归零后久坐自动按"稍后"处理。
    if (m_dialogVisible) {
        const int32_t remainMs = static_cast<int32_t>(m_alertDeadlineMs - nowMs);
        if (remainMs <= 0) {
            if (m_alertKind == hb::AlertKind::Sedentary && m_callbacks.snooze) {
                m_callbacks.snooze();
            }
            dismissAlert();
        } else {
            const int seconds = (remainMs + 999) / 1000;
            if (seconds != m_lastCountdown) {
                m_lastCountdown = seconds;
                if (m_callbacks.updateAlertCountdown) {
                    m_callbacks.updateAlertCountdown(seconds);
                }
            }
        }
    }

    // 4) 导航变化则切屏（切屏内部会重建状态栏、页点、弹层）。
    if (m_nav.current() != m_screens.active()) {
        showPage(m_nav.current(), true);
    }

    // 5) 每秒整页刷新一次；刚切屏/首帧则立刻刷新。
    if (m_dirty || nowMs - m_lastRefreshMs >= 1000u) {
        m_lastRefreshMs = nowMs;
        m_dirty = false;
        m_screens.refresh(snap);
        updateChrome(snap);
    }
}

void UiService::setModel(const Model& model) {
    std::lock_guard<std::mutex> guard(m_modelMutex);
    m_model = model;
}

Model UiService::snapshot() const {
    std::lock_guard<std::mutex> guard(m_modelMutex);
    return m_model;
}

void UiService::postEvent(const hb::Event& event) { m_queue.push(event); }

void UiService::setCallbacks(const UiCallbacks& callbacks) {
    std::lock_guard<std::mutex> guard(m_modelMutex);
    m_callbacks = callbacks;
}

void UiService::dismissAlert() {
    if (!m_dialogVisible) {
        return;
    }
    m_dialogVisible = false;
    m_alertKind = hb::AlertKind::None;
    m_lastCountdown = -1;
    m_nav.setLocked(false);
    if (m_dialogLayer != nullptr) {
        lv_obj_add_flag(m_dialogLayer, LV_OBJ_FLAG_HIDDEN);
    }
    if (m_callbacks.hideAlert) {
        m_callbacks.hideAlert();
    }
}

void UiService::handleEvent(const hb::Event& event, uint32_t nowMs) {
    Screen* screen = m_screens.activeScreen();
    if (screen != nullptr) {
        screen->onEvent(event);  // 页面可以先响应（例如弹出设置子项）
    }

    switch (event.type) {
        case hb::EventType::KeyShort:
            m_nav.next();
            break;
        case hb::EventType::KeyLong: {
            // 长按 = 开关屏幕：0 表示背光关，下一次长按恢复模型亮度。
            const int restore = static_cast<int>(snapshot().brightness);
            applyBrightness(m_brightness > 0 ? 0 : (restore > 0 ? restore : 80));
            break;
        }
        case hb::EventType::KeyVeryLong:
            m_nav.home();
            break;
        case hb::EventType::Alert:
            if (event.alert != hb::AlertKind::None) {
                openAlert(event.alert, event.value, nowMs);
            }
            break;
        case hb::EventType::UserStandUp:
        case hb::EventType::Snooze:
        case hb::EventType::AlertTimeout:
            dismissAlert();
            break;
        default:
            break;
    }
}

void UiService::openAlert(hb::AlertKind kind, int value, uint32_t nowMs) {
    if (m_dialogVisible || m_dialogLayer == nullptr) {
        return;
    }
    const int seconds = kind == hb::AlertKind::Fall ? kFallSec : kSnoozeSec;
    m_dialogVisible = true;
    m_alertKind = kind;
    m_alertDeadlineMs = nowMs + static_cast<uint32_t>(seconds) * 1000u;
    m_lastCountdown = seconds;
    lv_obj_remove_flag(m_dialogLayer, LV_OBJ_FLAG_HIDDEN);
    m_nav.setLocked(true);  // 弹窗期间按键不切页，避免误触打断提醒

    if (m_callbacks.showAlert) {
        m_callbacks.showAlert(kind, value);
    }
    if (m_callbacks.updateAlertCountdown) {
        m_callbacks.updateAlertCountdown(seconds);
    }
}

void UiService::showPage(PageId id, bool animate) {
    if (!m_screens.has(id)) {
        return;
    }
    if (m_screens.activeScreen() != nullptr && m_screens.active() == id) {
        return;
    }
    // 页面是独立 screen 对象，切换后旧屏上的状态栏/页点/弹层必须随旧屏销毁。
    detachChrome();
    m_screens.show(id, animate);
    Screen* screen = m_screens.activeScreen();
    attachChrome(screen != nullptr ? screen->screen() : nullptr);

    const Model snap = snapshot();
    updateChrome(snap);
    syncDots(static_cast<int>(m_screens.active()));
    m_screens.refresh(snap);
    m_dirty = true;
}

void UiService::attachChrome(lv_obj_t* host) {
    if (host == nullptr || m_statusBar != nullptr) {
        return;
    }

    // 状态栏：时间 / 久坐状态 / 电量
    m_statusBar = lv_obj_create(host);
    lv_obj_remove_style(m_statusBar, nullptr, 0);
    lv_obj_set_pos(m_statusBar, 0, 0);
    lv_obj_set_size(m_statusBar, kScreenWidth, kStatusBarPx);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(m_statusBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_statusBar, 0, 0);
    lv_obj_set_style_radius(m_statusBar, 0, 0);
    lv_obj_set_style_pad_all(m_statusBar, 0, 0);
    lv_obj_remove_flag(m_statusBar, LV_OBJ_FLAG_SCROLLABLE);

    m_statusTime = makeLabel(m_statusBar, "--:--", textFont(), kColorText, kSafePx, 16);
    m_statusState = makeLabel(m_statusBar, "", textFont(), kColorTextMuted, 128, 16);
    m_statusBattery = makeLabel(m_statusBar, "", textFont(), kColorTextMuted, 704, 16);

    // 页点栏：6 个圆点指示当前位置
    m_dotsBar = lv_obj_create(host);
    lv_obj_remove_style(m_dotsBar, nullptr, 0);
    lv_obj_set_pos(m_dotsBar, 0, kContentBottomPx);
    lv_obj_set_size(m_dotsBar, kScreenWidth, kDotsBarPx);
    lv_obj_set_style_bg_opa(m_dotsBar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m_dotsBar, 0, 0);
    lv_obj_set_style_pad_all(m_dotsBar, 0, 0);
    lv_obj_remove_flag(m_dotsBar, LV_OBJ_FLAG_SCROLLABLE);

    const int totalWidth = kPageCount * kDotGapPx;
    const int startX = (kScreenWidth - totalWidth) / 2;
    const int dotY = (kDotsBarPx - kDotPx) / 2;
    for (int i = 0; i < kPageCount; ++i) {
        m_dot[i] = makeBar(m_dotsBar, startX + i * kDotGapPx, dotY, kDotPx, kDotPx,
                           kColorCardBorder);
    }

    // 弹层：全屏、透明、默认隐藏；Dialog 由 hb_app 的 AlertOverlay 在其上创建
    m_dialogLayer = lv_obj_create(host);
    lv_obj_remove_style(m_dialogLayer, nullptr, 0);
    lv_obj_set_pos(m_dialogLayer, 0, 0);
    lv_obj_set_size(m_dialogLayer, kScreenWidth, kScreenHeight);
    lv_obj_set_style_bg_opa(m_dialogLayer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m_dialogLayer, 0, 0);
    lv_obj_set_style_pad_all(m_dialogLayer, 0, 0);
    lv_obj_remove_flag(m_dialogLayer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(m_dialogLayer, LV_OBJ_FLAG_HIDDEN);
}

void UiService::detachChrome() {
    if (m_dialogVisible) {
        dismissAlert();  // 先让弹窗自己收尾，再删它所在的层
    }
    compat::deleteObj(m_dialogLayer);
    compat::deleteObj(m_dotsBar);
    compat::deleteObj(m_statusBar);
    m_dialogLayer = nullptr;
    m_dotsBar = nullptr;
    m_statusBar = nullptr;
    m_statusTime = nullptr;
    m_statusState = nullptr;
    m_statusBattery = nullptr;
    for (int i = 0; i < kPageCount; ++i) {
        m_dot[i] = nullptr;
    }
}

void UiService::updateChrome(const Model& model) {
    char buffer[32];
    if (m_statusTime != nullptr && fmtClock(model.epochSec, buffer, sizeof(buffer))) {
        setText(m_statusTime, buffer);
    }
    if (m_statusState != nullptr) {
        if (!model.worn) {
            setText(m_statusState, "未佩戴");
        } else if ((model.sit == SitPhase::Sitting || model.sit == SitPhase::Snoozed) &&
                   fmtSitting(model.sittingMin, buffer, sizeof(buffer))) {
            char line[64];
            std::snprintf(line, sizeof(line), "久坐 %s 分钟", buffer);
            setText(m_statusState, line);
        } else {
            setText(m_statusState, sitPhaseText(model.sit));
        }
    }
    if (m_statusBattery != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%u%%", static_cast<unsigned>(model.batteryPct));
        setText(m_statusBattery, buffer);
    }
}

void UiService::syncDots(int index) {
    for (int i = 0; i < kPageCount; ++i) {
        if (m_dot[i] == nullptr) {
            continue;
        }
        setColor(m_dot[i], i == index ? kColorSport : kColorCardBorder);
    }
}

void UiService::applyBrightness(int pct) {
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    if (pct == m_brightness) {
        return;  // 值没变就不动背光，避免每帧刷 I2C
    }
    m_brightness = pct;
    if (m_config.backlight != nullptr && m_config.backlight->setPct) {
        m_config.backlight->setPct(pct);
    }
}

}  // namespace hb::ui
