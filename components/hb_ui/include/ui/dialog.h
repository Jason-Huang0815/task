#pragma once

#include <functional>

#include "lvgl.h"

// 业务弹窗：60% 黑遮罩 + 居中卡片 + 两个动作按钮。
// 久坐提醒（60 s，青绿）与跌倒确认（15 s，红）共用这一份实现，
// 由 hb_app 的 AlertOverlay 决定文案、颜色和按钮动作。

namespace hb::ui {

struct DialogSpec {
    const char* title = "";
    const char* body = "";
    const char* primaryLabel = "";
    const char* secondaryLabel = "";
    uint32_t accent = 0x2DD4BF;
    std::function<void()> onPrimary;
    std::function<void()> onSecondary;
};

class Dialog {
public:
    /// 在 parent（通常是 UiService::dialogLayer()）上建弹窗；重复 open 会先 close。
    lv_obj_t* open(lv_obj_t* parent, const DialogSpec& spec);

    /// 删除整棵弹窗对象树，可重复调用。
    void close();

    /// 更新"还剩 N 秒"；未打开时是空操作。
    void setCountdown(int seconds);

    bool visible() const { return m_root != nullptr; }
    lv_obj_t* root() const { return m_root; }

private:
    static void onPrimaryClicked(lv_event_t* event);
    static void onSecondaryClicked(lv_event_t* event);

    lv_obj_t* m_root = nullptr;
    lv_obj_t* m_countdown = nullptr;
    std::function<void()> m_onPrimary;
    std::function<void()> m_onSecondary;
};

}  // namespace hb::ui
