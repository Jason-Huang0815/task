#pragma once

#include "ui/screen.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 生命体征页：心率大数字、SpO2、信号质量三段条、24 点 PPG 波形。
// 心率数据未接入（hrPageEnabled == false）时中央显示"未启用"，数字显示 --。

namespace app {

class VitalScreen : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Heart; }
    const char* title() const override { return "心率"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

private:
    static constexpr int kWaveCount = 24;

    lv_obj_t* m_hr = nullptr;
    lv_obj_t* m_spo2 = nullptr;
    lv_obj_t* m_quality[3] = {nullptr};
    lv_obj_t* m_wave[kWaveCount] = {nullptr};
    lv_obj_t* m_disabled = nullptr;
};

}  // namespace app
