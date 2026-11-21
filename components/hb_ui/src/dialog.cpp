#include "ui/dialog.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// 弹窗实现：60% 黑遮罩 + 居中卡片 + 主/次两个按钮。
// 久坐与跌倒共用同一份布局，颜色、文案、按钮动作由 DialogSpec 决定。

namespace hb::ui {
namespace {

constexpr int kCardW = 560;
constexpr int kCardH = 300;
constexpr int kDialogRadiusPx = 24;
constexpr int kAccentBarW = 6;
constexpr int kAccentBarH = 112;
constexpr int kButtonW = 216;

}  // namespace

lv_obj_t* Dialog::open(lv_obj_t* parent, const DialogSpec& spec) {
    close();
    if (parent == nullptr) {
        return nullptr;
    }
    m_onPrimary = spec.onPrimary;
    m_onSecondary = spec.onSecondary;

    lv_obj_t* root = lv_obj_create(parent);
    if (root == nullptr) {
        return nullptr;
    }
    lv_obj_remove_style(root, nullptr, 0);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, kScreenWidth, kScreenHeight);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, kMaskOpa, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);  // 遮罩吞掉点击，防止穿透到下层按钮
    m_root = root;

    const int cardX = (kScreenWidth - kCardW) / 2;
    const int cardY = (kScreenHeight - kCardH) / 2;
    lv_obj_t* card = makeCard(root, cardX, cardY, kCardW, kCardH);
    if (card != nullptr) {
        lv_obj_set_style_radius(card, kDialogRadiusPx, 0);
        lv_obj_set_style_border_width(card, 3, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(spec.accent), 0);
    }

    makeBar(card, 32, 28, kAccentBarW, kAccentBarH, spec.accent);
    makeLabel(card, spec.title, titleFont(), spec.accent, 56, 26);
    makeLabel(card, spec.body, bodyFont(), kColorText, 56, 78);
    m_countdown = makeLabel(card, "", bodyFont(), kColorTextMuted, 56, 128);

    const int buttonY = kCardH - kButtonHeightPx - 28;
    lv_obj_t* primary =
        makeButton(card, 32, buttonY, kButtonW, kButtonHeightPx, spec.primaryLabel, spec.accent);
    if (primary != nullptr) {
        lv_obj_add_event_cb(primary, &Dialog::onPrimaryClicked, LV_EVENT_CLICKED, this);
    }

    // 次按钮：深色底 + 浅色字，和主按钮形成主次关系。
    lv_obj_t* secondary =
        makeButton(card, kCardW - kButtonW - 32, buttonY, kButtonW, kButtonHeightPx,
                   spec.secondaryLabel, kColorCardBorder, kColorText);
    if (secondary != nullptr) {
        lv_obj_add_event_cb(secondary, &Dialog::onSecondaryClicked, LV_EVENT_CLICKED, this);
    }
    return m_root;
}

void Dialog::close() {
    // 回调里可能已经删掉了祖先对象，这里只在本地指针非空时删。
    if (m_root != nullptr) {
        lv_obj_del(m_root);
        m_root = nullptr;
    }
    m_countdown = nullptr;
    m_onPrimary = nullptr;
    m_onSecondary = nullptr;
}

void Dialog::setCountdown(int seconds) {
    if (m_countdown == nullptr) {
        return;
    }
    char buffer[48];
    const int value = seconds > 0 ? seconds : 0;
    std::snprintf(buffer, sizeof(buffer), "还剩 %d 秒", value);
    lv_label_set_text(m_countdown, buffer);
}

void Dialog::onPrimaryClicked(lv_event_t* event) {
    Dialog* self = static_cast<Dialog*>(lv_event_get_user_data(event));
    if (self == nullptr) {
        return;
    }
    // 先拷贝动作再调用：动作会销毁弹窗（含当前按钮）。
    const std::function<void()> action = self->m_onPrimary;
    if (action) {
        action();
    }
}

void Dialog::onSecondaryClicked(lv_event_t* event) {
    Dialog* self = static_cast<Dialog*>(lv_event_get_user_data(event));
    if (self == nullptr) {
        return;
    }
    const std::function<void()> action = self->m_onSecondary;
    if (action) {
        action();
    }
}

}  // namespace hb::ui
