#pragma once

#include "ui/screen.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 体温页：当前温度、与基线的偏差、24 小时趋势（24 根竖条）。
// tempPageEnabled == false 时中央显示"未启用"，温度显示 --。

namespace app {

class TempScreen : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Temp; }
    const char* title() const override { return "体温"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

private:
    static constexpr int kTrendCount = 24;
    static constexpr int kTrendBaselineY = 330;
    static constexpr int kTrendMaxH = 120;

    void layoutTrend(const ui::Model& model);

    lv_obj_t* m_value = nullptr;
    lv_obj_t* m_delta = nullptr;
    lv_obj_t* m_deltaHint = nullptr;
    lv_obj_t* m_trend[kTrendCount] = {nullptr};
    lv_obj_t* m_disabled = nullptr;
};

}  // namespace app
