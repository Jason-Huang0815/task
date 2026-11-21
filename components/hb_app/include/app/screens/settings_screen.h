#pragma once

#include "ui/screen.h"
#include "ui/ui_service.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 设置页：五行 752×64 —— 亮度 / 阈值 / 震动 / 抬腕 / 立即同步。
// ± 按钮 56×48，改动立刻回显并回调；持久化（NVS）由角色 F 接。

namespace app {

class SettingsScreen : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Settings; }
    const char* title() const override { return "设置"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

    void setCallbacks(const ui::UiCallbacks& callbacks);

private:
    enum Row : int {
        kBrightness = 0,
        kThreshold = 1,
        kVibration = 2,
        kWrist = 3,
        kSync = 4,
        kRowCount = 5,
    };

    static void onTapped(lv_event_t* event);

    void handleTap(lv_obj_t* target);
    void adjust(int row, int direction);
    void updateLabels();

    ui::UiCallbacks m_callbacks;
    int m_amount[kRowCount] = {80, 45, 2, 0, 0};
    bool m_wristOn = true;
    lv_obj_t* m_value[kRowCount] = {nullptr};
    lv_obj_t* m_minus[kRowCount] = {nullptr};
    lv_obj_t* m_plus[kRowCount] = {nullptr};
};

}  // namespace app
