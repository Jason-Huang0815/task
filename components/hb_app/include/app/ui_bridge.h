#pragma once

#include <cstdint>

#include "app/app_controller.h"
#include "app/screens/alert_overlay.h"
#include "app/screens/sedentary_screen.h"
#include "app/screens/settings_screen.h"
#include "app/screens/sport_screen.h"
#include "app/screens/temp_screen.h"
#include "app/screens/vital_screen.h"
#include "app/screens/watch_face.h"
#include "common/events.h"
#include "common/settings.h"
#include "service/event_bus.h"
#include "service/sedentary_service.h"
#include "service/step_service.h"
#include "ui/model.h"
#include "ui/port.h"
#include "ui/ui_service.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 界面桥：把 service 数据整理成 ui::Model，把按钮动作翻译成 service 调用与事件。
// 本层不持有业务状态，也不做 NVS 持久化（由角色 F 接）。

namespace app {

class UiBridge {
public:
    /// settings 必须与 StepService / SedentaryService 用的是同一个实例，否则阈值改动不生效。
    UiBridge(hb::EventBus& bus, hb::StepService& steps, hb::SedentaryService& sedentary,
             hb::Settings& settings);

    bool begin(lv_obj_t* statusHost, const ui::Port& port, const ui::BacklightPort& backlight);

    /// ui_task 里每帧调用：先刷新模型再驱动界面。
    void tick(uint32_t nowMs);

    /// 业务层事件入口（键盘、告警、佩戴状态……），转投界面队列。
    void onEvent(const hb::Event& event);

    /// 同步状态由角色 F 回填：1 = 同步中，2 = 已同步。
    void setSyncState(uint8_t state);

    ui::UiService& ui() { return m_ui; }

private:
    ui::Model buildModel() const;
    ui::UiCallbacks makeCallbacks();
    void publish(hb::EventType type, int32_t value, hb::AlertKind alert = hb::AlertKind::None);

    hb::EventBus& m_bus;
    hb::StepService& m_steps;
    hb::SedentaryService& m_sedentary;
    hb::Settings& m_settings;

    ui::Port m_port;
    ui::BacklightPort m_backlight;
    ui::UiService m_ui;
    app::AlertOverlay m_overlay;
    app::WatchFace m_watchFace;
    app::SportScreen m_sport;
    app::VitalScreen m_vital;
    app::TempScreen m_temp;
    app::SedentaryScreen m_sedentaryPage;
    app::SettingsScreen m_settingsPage;

    uint8_t m_syncState = 0;
    uint32_t m_lastTickMs = 0;
    uint32_t m_activeMs = 0;
    uint16_t m_activeMin = 0;
};

}  // namespace app
