#include "app/screens/settings_screen.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 设置页实现：五行 752×64，单行内 [标题 | 数值 | ± 按钮]。
// 值改动立刻回显并回调；界面上限与 Settings 的取值范围保持一致。

namespace app {
namespace {

const char* const kRowNames[5] = {"亮度", "久坐阈值", "震动强度", "抬腕亮屏", "立即同步"};

constexpr int kFirstRowY = 60;
constexpr int kRowStepY = 68;
constexpr int kRowX = 24;
constexpr int kRowW = 752;
constexpr int kRowH = 64;
constexpr int kValueX = 360;
constexpr int kValueW = 220;
constexpr int kMinusX = 596;
constexpr int kPlusX = 664;
constexpr int kStepButtonW = 56;
constexpr int kStepButtonH = 48;
constexpr int kStepButtonY = 8;
constexpr int kSyncButtonW = 112;

int clampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

}  // namespace

void SettingsScreen::onCreate(lv_obj_t* parent) {
    for (int row = 0; row < kRowCount; ++row) {
        const int y = kFirstRowY + row * kRowStepY;
        lv_obj_t* line = ui::makeCard(parent, kRowX, y, kRowW, kRowH);
        ui::makeLabel(line, kRowNames[row], ui::bodyFont(), ui::kColorText, 20, 22);
        m_value[row] = ui::makeValue(line, ui::bodyFont(), ui::kColorSport, kValueX, 22);
        lv_obj_set_width(m_value[row], kValueW);
        lv_obj_set_style_text_align(m_value[row], LV_TEXT_ALIGN_RIGHT, 0);

        if (row == kSync) {
            // 同步行只给一个动作按钮，值由 fmtSync 决定（待同步/同步中/已同步）。
            m_plus[row] = ui::makeButton(line, kPlusX, kStepButtonY, kSyncButtonW, kStepButtonH,
                                         "同步", ui::kColorSport);
            lv_obj_add_event_cb(m_plus[row], &SettingsScreen::onTapped, LV_EVENT_CLICKED, this);
            continue;
        }

        m_minus[row] = ui::makeButton(line, kMinusX, kStepButtonY, kStepButtonW, kStepButtonH, "-",
                                      ui::kColorCardBorder, ui::kColorText);
        m_plus[row] = ui::makeButton(line, kPlusX, kStepButtonY, kStepButtonW, kStepButtonH, "+",
                                     ui::kColorSport);
        lv_obj_add_event_cb(m_minus[row], &SettingsScreen::onTapped, LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(m_plus[row], &SettingsScreen::onTapped, LV_EVENT_CLICKED, this);
    }
    updateLabels();
}

void SettingsScreen::setCallbacks(const ui::UiCallbacks& callbacks) { m_callbacks = callbacks; }

void SettingsScreen::onEvent(const hb::Event& event) { (void)event; }

void SettingsScreen::onTapped(lv_event_t* event) {
    SettingsScreen* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
    if (self == nullptr) {
        return;
    }
    self->handleTap(static_cast<lv_obj_t*>(lv_event_get_target(event)));
}

void SettingsScreen::handleTap(lv_obj_t* target) {
    if (target == nullptr) {
        return;
    }
    if (target == m_plus[kSync]) {
        if (m_callbacks.syncNow) {
            m_callbacks.syncNow();
        }
        return;
    }
    for (int row = 0; row < kSync; ++row) {
        if (target == m_minus[row]) {
            adjust(row, -1);
            return;
        }
        if (target == m_plus[row]) {
            adjust(row, +1);
            return;
        }
    }
}

void SettingsScreen::adjust(int row, int direction) {
    switch (row) {
        case kBrightness: {
            const int value = clampInt(m_amount[row] + direction * 10, 1, 100);
            m_amount[row] = value;
            if (m_callbacks.setBrightness) {
                m_callbacks.setBrightness(value);
            }
            break;
        }
        case kThreshold: {
            const int value = clampInt(m_amount[row] + direction * 5, 10, 120);
            m_amount[row] = value;
            if (m_callbacks.setThreshold) {
                m_callbacks.setThreshold(value);
            }
            break;
        }
        case kVibration: {
            const int value = clampInt(m_amount[row] + direction, 0, 3);
            m_amount[row] = value;
            if (m_callbacks.setVibration) {
                m_callbacks.setVibration(value);
            }
            break;
        }
        case kWrist: {
            m_wristOn = direction > 0;
            if (m_callbacks.setWristRaise) {
                m_callbacks.setWristRaise(m_wristOn);
            }
            break;
        }
        default:
            break;
    }
    updateLabels();
}

void SettingsScreen::updateLabels() {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d%%", m_amount[kBrightness]);
    ui::setText(m_value[kBrightness], buffer);
    std::snprintf(buffer, sizeof(buffer), "%d 分钟", m_amount[kThreshold]);
    ui::setText(m_value[kThreshold], buffer);
    std::snprintf(buffer, sizeof(buffer), "%d 档", m_amount[kVibration]);
    ui::setText(m_value[kVibration], buffer);
    ui::setText(m_value[kWrist], m_wristOn ? "开" : "关");
}

void SettingsScreen::refresh(const ui::Model& model) {
    m_amount[kBrightness] = model.brightness;
    m_amount[kThreshold] = model.thresholdMin;
    m_amount[kVibration] = model.vibrationLevel;
    m_wristOn = model.wristRaise;
    updateLabels();

    char buffer[32];
    if (ui::fmtSync(model.syncState, buffer, sizeof(buffer))) {
        ui::setText(m_value[kSync], buffer);
    }
}

}  // namespace app
