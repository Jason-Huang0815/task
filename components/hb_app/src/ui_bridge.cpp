#include "app/ui_bridge.h"

#include <ctime>

#include "ui/theme.h"

#if defined(ESP_PLATFORM)
#include "esp_timer.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;
#endif

// 桥接实现：service -> ui::Model，按钮回调 -> service 调用 + 事件。
// 所有传感器缺省值都在这里显式置零并关闭对应 *PageEnabled，页面据此显示"未启用"。

namespace app {
namespace {

constexpr uint32_t kMaxTickGapMs = 5000;      // 超过这个间隔视为休眠，不计活跃时长
constexpr float kActiveCadence = 30.0f;       // 步频达到 30 步/分才算活跃
constexpr int16_t kTempBaselineX100 = 3650;   // 36.5 ℃，体温基线（角色 F 接入后改为个体基线）

int clampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

int64_t nowUs() {
#if defined(ESP_PLATFORM)
    return esp_timer_get_time();
#else
    return static_cast<int64_t>(std::time(nullptr)) * 1000000;
#endif
}

ui::SitPhase sitPhaseOf(hb::SitState state) {
    switch (state) {
        case hb::SitState::WearOff:
            return ui::SitPhase::WearOff;
        case hb::SitState::Active:
            return ui::SitPhase::Active;
        case hb::SitState::Sitting:
            return ui::SitPhase::Sitting;
        case hb::SitState::Alerting:
            return ui::SitPhase::Alerting;
        case hb::SitState::Snoozed:
            return ui::SitPhase::Snoozed;
        case hb::SitState::Quiet:
            return ui::SitPhase::Quiet;
    }
    return ui::SitPhase::WearOff;
}

}  // namespace

UiBridge::UiBridge(hb::EventBus& bus, hb::StepService& steps, hb::SedentaryService& sedentary,
                   hb::Settings& settings)
    : m_bus(bus), m_steps(steps), m_sedentary(sedentary), m_settings(settings) {}

bool UiBridge::begin(lv_obj_t* statusHost, const ui::Port& port,
                     const ui::BacklightPort& backlight) {
    m_port = port;
    m_backlight = backlight;

    ui::UiConfig config;
    config.port = &m_port;
    config.backlight = &m_backlight;
    config.brightnessDefaultPct = clampInt(m_settings.brightness, 1, 100);

    if (!m_ui.begin(statusHost, config)) {
        return false;
    }
    m_ui.addScreen(&m_watchFace);
    m_ui.addScreen(&m_sport);
    m_ui.addScreen(&m_vital);
    m_ui.addScreen(&m_temp);
    m_ui.addScreen(&m_sedentaryPage);
    m_ui.addScreen(&m_settingsPage);

    const ui::UiCallbacks callbacks = makeCallbacks();
    m_ui.setCallbacks(callbacks);
    m_sedentaryPage.setCallbacks(callbacks);
    m_settingsPage.setCallbacks(callbacks);

    m_ui.setModel(buildModel());
    m_ui.start();  // 首次装载首页会重建状态栏与弹层，因此弹层绑定放在 start 之后

    m_overlay.create(&m_ui);
    m_overlay.setCallbacks(callbacks);
    return true;
}

void UiBridge::tick(uint32_t nowMs) {
    uint32_t deltaMs = nowMs - m_lastTickMs;
    m_lastTickMs = nowMs;
    if (deltaMs > kMaxTickGapMs) {
        deltaMs = 0;
    }
    if (m_steps.cadence() >= kActiveCadence) {
        m_activeMs += deltaMs;
        while (m_activeMs >= 60000u) {
            m_activeMs -= 60000u;
            if (m_activeMin < 0xFFFFu) {
                ++m_activeMin;
            }
        }
    }
    m_ui.setModel(buildModel());
    m_ui.tick(nowMs);
}

void UiBridge::onEvent(const hb::Event& event) { m_ui.postEvent(event); }

void UiBridge::setSyncState(uint8_t state) { m_syncState = state; }

ui::Model UiBridge::buildModel() const {
    ui::Model model;
    model.epochSec = static_cast<uint32_t>(std::time(nullptr));

    const uint32_t steps = m_steps.steps();
    model.steps = static_cast<uint16_t>(steps > 0xFFFFu ? 0xFFFFu : steps);

    const float cadence = m_steps.cadence();
    model.cadence = static_cast<uint16_t>(clampInt(static_cast<int>(cadence), 0, 300));

    // 心率 / 血氧 / 体温：传感器由角色 F 接入，这里不做假数据。
    model.hr = 0;
    model.spo2 = 0;
    model.tempX100 = 0;
    model.hrPageEnabled = false;
    model.tempPageEnabled = false;
    model.hrValid = false;
    model.spo2Valid = false;
    model.tempValid = false;
    model.tempBaselineX100 = kTempBaselineX100;

    const hb::SitState state = m_sedentary.state();
    model.sit = sitPhaseOf(state);
    model.sittingMin = static_cast<uint16_t>(clampInt(m_sedentary.sittingMinutes(), 0, 1440));
    model.worn = state != hb::SitState::WearOff;
    model.snoozeCount = static_cast<uint8_t>(clampInt(m_sedentary.snoozeCount(), 0, 255));
    model.snoozeLeftMin =
        state == hb::SitState::Snoozed
            ? static_cast<uint8_t>(clampInt(m_settings.snoozeMin, 0, 255))
            : 0;
    model.alerting = m_sedentary.isAlerting();

    model.batteryPct = 100;  // 电量 ADC 由角色 F 接
    model.brightness = static_cast<uint8_t>(clampInt(m_settings.brightness, 0, 100));
    model.thresholdMin = static_cast<uint8_t>(clampInt(m_settings.thresholdMin, 0, 120));
    model.vibrationLevel = static_cast<uint8_t>(clampInt(m_settings.vibrationLevel, 0, 3));
    model.wristRaise = m_settings.wristRaise;
    model.syncState = m_syncState;
    model.btConnected = false;

    model.distanceM = static_cast<uint32_t>(model.steps) * 7u / 10u;  // 每步 0.7 m
    model.kcal = static_cast<uint16_t>(model.steps / 30);             // 每 30 步 1 kcal
    model.activeMin = m_activeMin;

    for (int i = 0; i < 7; ++i) {
        model.sevenDay[i] = 0;  // 历史记录由角色 F 从 RecordStore 回填
    }
    model.sevenDay[6] = model.steps;
    return model;
}

ui::UiCallbacks UiBridge::makeCallbacks() {
    ui::UiCallbacks callbacks;

    callbacks.standUp = [this]() {
        // 先更新服务再发事件：服务在非提醒态直接返回，重复调用是幂等的。
        m_sedentary.onUserStandUp(nowUs());
        publish(hb::EventType::UserStandUp, 0);
        m_ui.dismissAlert();
    };
    callbacks.snooze = [this]() {
        m_sedentary.onSnooze(nowUs());
        publish(hb::EventType::Snooze, 0);
        m_ui.dismissAlert();
    };
    callbacks.syncNow = [this]() {
        // 本层不持有待同步记录：没有积压时同步是幂等的，直接就是"已同步"。
        m_syncState = 2;
    };
    callbacks.setBrightness = [this](int value) {
        m_settings.brightness = clampInt(value, 1, 100);
    };
    callbacks.setThreshold = [this](int value) {
        m_settings.thresholdMin = clampInt(value, 10, 120);
    };
    callbacks.setVibration = [this](int value) {
        m_settings.vibrationLevel = clampInt(value, 0, 3);
    };
    callbacks.setWristRaise = [this](bool on) { m_settings.wristRaise = on; };
    callbacks.showAlert = [this](hb::AlertKind kind, int value) { m_overlay.show(kind, value); };
    callbacks.updateAlertCountdown = [this](int seconds) { m_overlay.updateCountdown(seconds); };
    callbacks.hideAlert = [this]() { m_overlay.hide(); };
    callbacks.fallSos = [this]() {
        // 求助通道：与长按 SOS 手势沿用同一个事件，alert 字段标记来源。
        publish(hb::EventType::KeyVeryLong, 0, hb::AlertKind::Fall);
        m_ui.dismissAlert();
    };
    return callbacks;
}

void UiBridge::publish(hb::EventType type, int32_t value, hb::AlertKind alert) {
    hb::Event event{};
    event.type = type;
    event.value = value;
    event.alert = alert;
    m_bus.publish(event);
}

}  // namespace app
