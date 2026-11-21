#pragma once

#include "ui/screen.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 运动页：380×200 步数卡（含距离/热量/活跃时长）、220 px 目标环、最近 7 天柱状图。

namespace app {

class SportScreen : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Sport; }
    const char* title() const override { return "运动"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

private:
    static constexpr int kDayCount = 7;
    static constexpr int kBarMaxH = 96;
    static constexpr int kBarBaselineY = 140;

    void layoutBars(const ui::Model& model);

    lv_obj_t* m_steps = nullptr;
    lv_obj_t* m_distance = nullptr;
    lv_obj_t* m_kcal = nullptr;
    lv_obj_t* m_active = nullptr;
    lv_obj_t* m_ring = nullptr;
    lv_obj_t* m_ringPct = nullptr;
    lv_obj_t* m_goal = nullptr;
    lv_obj_t* m_day[kDayCount] = {nullptr};
};

}  // namespace app
