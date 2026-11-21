#include "app/screens/sport_screen.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 运动页实现：步数卡 + 220 px 目标环 + 7 天柱状图。
// 目标值取产品默认 8000 步（模型暂不携带目标，角色 F 接入档案后替换）。

namespace app {
namespace {

constexpr int kGoalSteps = 8000;
constexpr int kBarW = 28;
constexpr int kBarGap = 48;

}  // namespace

void SportScreen::onCreate(lv_obj_t* parent) {
    lv_obj_t* stepsCard = ui::makeCard(parent, 24, 72, 380, 200);
    ui::makeLabel(stepsCard, "今日步数", ui::bodyFont(), ui::kColorTextMuted, 24, 12);
    m_steps = ui::makeValue(stepsCard, ui::numberFont(), ui::kColorSport, 24, 34);
    ui::makeLabel(stepsCard, "步", ui::bodyFont(), ui::kColorTextMuted, 206, 60);

    ui::makeLabel(stepsCard, "距离", ui::bodyFont(), ui::kColorTextMuted, 24, 126);
    m_distance = ui::makeValue(stepsCard, ui::bodyFont(), ui::kColorText, 24, 150);
    ui::makeLabel(stepsCard, "热量", ui::bodyFont(), ui::kColorTextMuted, 148, 126);
    m_kcal = ui::makeValue(stepsCard, ui::bodyFont(), ui::kColorText, 148, 150);
    ui::makeLabel(stepsCard, "活跃", ui::bodyFont(), ui::kColorTextMuted, 272, 126);
    m_active = ui::makeValue(stepsCard, ui::bodyFont(), ui::kColorText, 272, 150);

    lv_obj_t* ringCard = ui::makeCard(parent, 420, 72, 356, 368);
    ui::makeLabel(ringCard, "每日目标", ui::titleFont(), ui::kColorText, 24, 18);
    m_ring = ui::makeArc(ringCard, 68, 66, 220, 18, ui::kColorSport, ui::kColorCardBorder);
    m_ringPct = ui::makeValue(ringCard, ui::titleFont(), ui::kColorSport, 146, 148);
    m_goal = ui::makeLabel(ringCard, "目标 8000 步", ui::bodyFont(), ui::kColorTextMuted, 86, 300);

    lv_obj_t* weekCard = ui::makeCard(parent, 24, 288, 380, 152);
    ui::makeLabel(weekCard, "最近 7 天", ui::bodyFont(), ui::kColorTextMuted, 24, 10);
    for (int i = 0; i < kDayCount; ++i) {
        m_day[i] = ui::makeBar(weekCard, 24 + i * kBarGap, kBarBaselineY - 4, kBarW, 4,
                               ui::kColorCardBorder);
    }
}

void SportScreen::onEvent(const hb::Event& event) { (void)event; }

void SportScreen::layoutBars(const ui::Model& model) {
    uint16_t peak = 1;
    for (int i = 0; i < kDayCount; ++i) {
        if (model.sevenDay[i] > peak) {
            peak = model.sevenDay[i];
        }
    }
    for (int i = 0; i < kDayCount; ++i) {
        int height = static_cast<int>(model.sevenDay[i]) * kBarMaxH / peak;
        if (height < 4) {
            height = 4;
        }
        lv_obj_t* bar = m_day[i];
        if (bar == nullptr) {
            continue;
        }
        lv_obj_set_pos(bar, 24 + i * kBarGap, kBarBaselineY - height);
        lv_obj_set_size(bar, kBarW, height);
        // 今天是最后一根，用主题青绿高亮；其余历史日灰蓝。
        ui::setColor(bar, i == kDayCount - 1 ? ui::kColorSport : ui::kColorCardBorder);
    }
}

void SportScreen::refresh(const ui::Model& model) {
    char buffer[32];
    char line[48];
    if (ui::fmtSteps(model.steps, buffer, sizeof(buffer))) {
        ui::setText(m_steps, buffer);
    }
    if (ui::fmtDistance(model.distanceM, buffer, sizeof(buffer))) {
        std::snprintf(line, sizeof(line), "%s 公里", buffer);
        ui::setText(m_distance, line);
    }
    if (ui::fmtKcal(model.kcal, buffer, sizeof(buffer))) {
        std::snprintf(line, sizeof(line), "%s 千卡", buffer);
        ui::setText(m_kcal, line);
    }
    if (ui::fmtActiveMin(model.activeMin, buffer, sizeof(buffer))) {
        std::snprintf(line, sizeof(line), "%s 分钟", buffer);
        ui::setText(m_active, line);
    }

    int percent = static_cast<int>(model.steps) * 100 / kGoalSteps;
    if (percent > 100) {
        percent = 100;
    }
    ui::setArcPercent(m_ring, percent);
    std::snprintf(line, sizeof(line), "%d%%", percent);
    ui::setText(m_ringPct, line);

    layoutBars(model);
}

}  // namespace app
