#include "app/screens/sedentary_screen.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 久坐页实现：左侧数据卡（分钟数 / 状态 / 两个按钮），右侧倒计时环。

namespace app {
namespace {

constexpr int kSnoozeMinutes = 10;

int percentOf(int value, int total) {
    if (total <= 0) {
        return 0;
    }
    int percent = value * 100 / total;
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    return percent;
}

}  // namespace

void SedentaryScreen::onCreate(lv_obj_t* parent) {
    lv_obj_t* left = ui::makeCard(parent, 24, 72, 460, 368);
    ui::makeLabel(left, "已坐时长", ui::bodyFont(), ui::kColorTextMuted, 28, 24);
    m_minutes = ui::makeValue(left, ui::clockFont(), ui::kColorSport, 28, 56);
    ui::makeLabel(left, "分钟", ui::bodyFont(), ui::kColorTextMuted, 212, 126);
    m_status = ui::makeValue(left, ui::titleFont(), ui::kColorText, 28, 196);
    m_hint = ui::makeLabel(left, "", ui::bodyFont(), ui::kColorTextMuted, 28, 234);

    m_standUp =
        ui::makeButton(left, 28, 288, 196, ui::kButtonHeightPx, "我起来了", ui::kColorSport);
    if (m_standUp != nullptr) {
        lv_obj_add_event_cb(m_standUp, &SedentaryScreen::onStandUpClicked, LV_EVENT_CLICKED, this);
    }
    m_snooze = ui::makeButton(left, 240, 288, 196, ui::kButtonHeightPx, "稍后 10 分钟",
                              ui::kColorCardBorder, ui::kColorText);
    if (m_snooze != nullptr) {
        lv_obj_add_event_cb(m_snooze, &SedentaryScreen::onSnoozeClicked, LV_EVENT_CLICKED, this);
    }

    lv_obj_t* right = ui::makeCard(parent, 500, 72, 276, 368);
    ui::makeLabel(right, "距提醒", ui::bodyFont(), ui::kColorTextMuted, 24, 22);
    m_arc = ui::makeArc(right, 38, 62, 200, 16, ui::kColorSport, ui::kColorCardBorder);
    m_arcValue = ui::makeValue(right, ui::titleFont(), ui::kColorText, 100, 148);
}

void SedentaryScreen::setCallbacks(const ui::UiCallbacks& callbacks) { m_callbacks = callbacks; }

void SedentaryScreen::onEvent(const hb::Event& event) { (void)event; }

void SedentaryScreen::onStandUpClicked(lv_event_t* event) {
    SedentaryScreen* self = static_cast<SedentaryScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->m_callbacks.standUp) {
        self->m_callbacks.standUp();
    }
}

void SedentaryScreen::onSnoozeClicked(lv_event_t* event) {
    SedentaryScreen* self = static_cast<SedentaryScreen*>(lv_event_get_user_data(event));
    if (self != nullptr && self->m_callbacks.snooze) {
        self->m_callbacks.snooze();
    }
}

void SedentaryScreen::refresh(const ui::Model& model) {
    char buffer[32];
    char line[48];

    if (ui::fmtSitting(model.sittingMin, buffer, sizeof(buffer))) {
        ui::setText(m_minutes, buffer);
    }
    ui::setText(m_status, ui::sitPhaseText(model.sit));

    int percent = 0;
    uint32_t arcColor = ui::kColorSport;
    switch (model.sit) {
        case ui::SitPhase::Alerting:
            percent = 100;
            arcColor = ui::kColorHeart;
            std::snprintf(line, sizeof(line), "该起来活动一下");
            break;
        case ui::SitPhase::Snoozed:
            percent = percentOf(model.snoozeLeftMin, kSnoozeMinutes);
            arcColor = ui::kColorTemp;
            std::snprintf(line, sizeof(line), "%u 分钟后重新提醒",
                          static_cast<unsigned>(model.snoozeLeftMin));
            break;
        default:
            percent = percentOf(model.sittingMin, model.thresholdMin);
            std::snprintf(line, sizeof(line), "每 %u 分钟提醒一次",
                          static_cast<unsigned>(model.thresholdMin));
            break;
    }
    ui::setText(m_hint, line);
    ui::setArcPercent(m_arc, percent);
    lv_obj_set_style_arc_color(m_arc, lv_color_hex(arcColor), 0);
    std::snprintf(line, sizeof(line), "%d%%", percent);
    ui::setText(m_arcValue, line);
}

}  // namespace app
