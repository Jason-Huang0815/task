#include "app/screens/alert_overlay.h"

#include <cstdio>

#include "ui/theme.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 弹层实现：把 AlertKind 翻译成 DialogSpec 的文案、颜色与两个动作。
// 动作只调 UiCallbacks，弹窗的开关仍由 UiService 的倒计时状态机控制。

namespace app {

void AlertOverlay::create(ui::UiService* ui) { m_ui = ui; }

void AlertOverlay::setCallbacks(const ui::UiCallbacks& callbacks) { m_callbacks = callbacks; }

void AlertOverlay::show(hb::AlertKind kind, int value) {
    if (m_ui == nullptr) {
        return;
    }
    lv_obj_t* layer = m_ui->dialogLayer();
    if (layer == nullptr) {
        return;
    }

    ui::DialogSpec spec;
    if (kind == hb::AlertKind::Fall) {
        spec.title = "检测到跌倒";
        spec.body = "需要帮助请立即求助";
        spec.primaryLabel = "我没事";
        spec.secondaryLabel = "立即求助";
        spec.accent = ui::kColorHeart;
        spec.onPrimary = [this]() {
            if (m_callbacks.standUp) {
                m_callbacks.standUp();
            }
        };
        spec.onSecondary = [this]() {
            if (m_callbacks.fallSos) {
                m_callbacks.fallSos();
            }
        };
    } else {
        // body 指向栈上缓冲：Dialog::open 会立刻把文字拷进 label，生命周期安全。
        char body[64];
        std::snprintf(body, sizeof(body), "已连续久坐 %d 分钟，起来活动一下",
                      value > 0 ? value : 0);
        spec.title = "久坐提醒";
        spec.body = body;
        spec.primaryLabel = "已起身";
        spec.secondaryLabel = "稍后 10 分钟";
        spec.accent = ui::kColorSport;
        spec.onPrimary = [this]() {
            if (m_callbacks.standUp) {
                m_callbacks.standUp();
            }
        };
        spec.onSecondary = [this]() {
            if (m_callbacks.snooze) {
                m_callbacks.snooze();
            }
        };
    }
    m_dialog.open(layer, spec);
}

void AlertOverlay::updateCountdown(int seconds) { m_dialog.setCountdown(seconds); }

void AlertOverlay::hide() { m_dialog.close(); }

}  // namespace app
