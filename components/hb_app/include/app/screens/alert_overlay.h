#pragma once

#include "common/events.h"
#include "ui/dialog.h"
#include "ui/ui_service.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 告警弹层：复用 ui::Dialog 实现两种弹窗。
// 久坐：青绿、"已起身 / 稍后 10 分钟"、60 秒；
// 跌倒：红色 #FF5A6E、"我没事 / 立即求助"、15 秒。

namespace app {

class AlertOverlay {
public:
    /// 绑定 UiService：弹层容器随切屏重建，所以每次显示时现取 dialogLayer()。
    void create(ui::UiService* ui);

    void setCallbacks(const ui::UiCallbacks& callbacks);
    void show(hb::AlertKind kind, int value);
    void updateCountdown(int seconds);
    void hide();

    bool visible() const { return m_dialog.visible(); }

private:
    ui::Dialog m_dialog;
    ui::UiCallbacks m_callbacks;
    ui::UiService* m_ui = nullptr;
};

}  // namespace app
