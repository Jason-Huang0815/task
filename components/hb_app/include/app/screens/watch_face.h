#pragma once

#include "ui/screen.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 表盘页：左侧 420×360 时间卡（112 px 等宽数字 + 日期 + 星期），
// 右侧三张 316×112 卡（步数 / 心率 / 体温）。

namespace app {

class WatchFace : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Watch; }
    const char* title() const override { return "表盘"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

private:
    lv_obj_t* m_time = nullptr;
    lv_obj_t* m_date = nullptr;
    lv_obj_t* m_weekday = nullptr;
    lv_obj_t* m_steps = nullptr;
    lv_obj_t* m_hr = nullptr;
    lv_obj_t* m_temp = nullptr;
};

}  // namespace app
