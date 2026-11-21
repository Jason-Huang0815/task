#include "app/screens/temp_screen.h"

#include <cstdio>

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 体温页实现：当前温度 + 基线偏差 + 24 h 趋势条。
// 模型只有当前温度，没有 24 点缓冲，趋势条固定按昼夜形状画；
// 角色 F 接入历史曲线后只需改 layoutTrend 的数据源。

namespace app {
namespace {

// 24 小时昼夜形状（相对高度，px）：凌晨低谷、傍晚高峰。
constexpr uint8_t kCircadian[24] = {26, 22, 20, 18, 18, 20, 24, 30, 38, 46, 54, 62,
                                    70, 78, 86, 92, 98, 104, 100, 92, 82, 72, 60, 48};

constexpr int kTrendX = 40;
constexpr int kTrendStep = 26;
constexpr int kTrendBarW = 12;

}  // namespace

void TempScreen::onCreate(lv_obj_t* parent) {
    lv_obj_t* card = ui::makeCard(parent, 24, 72, 752, 368);

    ui::makeLabel(card, "当前体温", ui::bodyFont(), ui::kColorTextMuted, 40, 26);
    m_value = ui::makeValue(card, ui::clockFont(), ui::kColorTemp, 40, 56);
    ui::makeLabel(card, "℃", ui::bodyFont(), ui::kColorTextMuted, 246, 126);

    ui::makeLabel(card, "与基线偏差", ui::bodyFont(), ui::kColorTextMuted, 460, 40);
    m_delta = ui::makeValue(card, ui::numberFont(), ui::kColorTemp, 460, 74);
    m_deltaHint = ui::makeLabel(card, "基线 36.5 ℃", ui::bodyFont(), ui::kColorTextMuted, 460, 140);

    ui::makeLabel(card, "24 小时趋势", ui::bodyFont(), ui::kColorTextMuted, 40, 182);
    for (int i = 0; i < kTrendCount; ++i) {
        m_trend[i] = ui::makeBar(card, kTrendX + i * kTrendStep, kTrendBaselineY - 8, kTrendBarW, 8,
                                 ui::kColorTemp);
    }

    m_disabled = ui::makeLabel(card, "未启用", ui::bodyFont(), ui::kColorTextMuted, 612, 22);
    ui::setVisible(m_disabled, false);
}

void TempScreen::onEvent(const hb::Event& event) { (void)event; }

void TempScreen::layoutTrend(const ui::Model& model) {
    for (int i = 0; i < kTrendCount; ++i) {
        int height = kCircadian[i];
        if (height > kTrendMaxH) {
            height = kTrendMaxH;
        }
        lv_obj_t* bar = m_trend[i];
        if (bar == nullptr) {
            continue;
        }
        lv_obj_set_pos(bar, kTrendX + i * kTrendStep, kTrendBaselineY - height);
        lv_obj_set_size(bar, kTrendBarW, height);
        ui::setColor(bar, model.tempPageEnabled ? ui::kColorTemp : ui::kColorCardBorder);
    }
}

void TempScreen::refresh(const ui::Model& model) {
    char buffer[32];
    if (ui::fmtTemp(model, buffer, sizeof(buffer))) {
        ui::setText(m_value, buffer);
    }

    if (model.tempPageEnabled && model.tempValid) {
        const int current = static_cast<int>(model.tempX100);
        const int baseline = static_cast<int>(model.tempBaselineX100);
        const int delta = current - baseline;
        char deltaText[16];
        if (ui::fmtDelta(static_cast<int16_t>(delta), deltaText, sizeof(deltaText))) {
            char line[24];
            std::snprintf(line, sizeof(line), "%s ℃", deltaText);
            ui::setText(m_delta, line);
        }
    } else {
        ui::setText(m_delta, "--");
    }

    // 基线提示跟随模型，便于核对偏差口径。
    char baseline[32];
    std::snprintf(baseline, sizeof(baseline), "基线 %d.%02d ℃", model.tempBaselineX100 / 100,
                  model.tempBaselineX100 % 100);
    ui::setText(m_deltaHint, baseline);

    ui::setVisible(m_disabled, !model.tempPageEnabled);
    layoutTrend(model);
}

}  // namespace app
