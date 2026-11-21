#pragma once

#include "ui/screen.h"
#include "ui/ui_service.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 久坐页：已坐分钟数、状态文字、倒计时环、两个 52 px 高按钮。
// 按钮只调 UiCallbacks，业务判定留给 SedentaryService。

namespace app {

class SedentaryScreen : public ui::Screen {
public:
    ui::PageId id() const override { return ui::PageId::Sedentary; }
    const char* title() const override { return "久坐"; }

    void onCreate(lv_obj_t* parent) override;
    void onEnter() override {}
    void onLeave() override {}
    void onEvent(const hb::Event& event) override;
    void refresh(const ui::Model& model) override;

    void setCallbacks(const ui::UiCallbacks& callbacks);

private:
    static void onStandUpClicked(lv_event_t* event);
    static void onSnoozeClicked(lv_event_t* event);

    ui::UiCallbacks m_callbacks;
    lv_obj_t* m_minutes = nullptr;
    lv_obj_t* m_status = nullptr;
    lv_obj_t* m_arc = nullptr;
    lv_obj_t* m_arcValue = nullptr;
    lv_obj_t* m_hint = nullptr;
    lv_obj_t* m_standUp = nullptr;
    lv_obj_t* m_snooze = nullptr;
};

}  // namespace app
