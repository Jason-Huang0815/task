#include "app/screens/vital_screen.h"

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 生命体征页实现：心率/血氧/信号质量/PPG 波形四块。
// 无 PPG 缓冲时波形是一张固定脉形示意，数值仍由 fmtHr/fmtSpo2 决定（未启用时显示 --）。

namespace app {
namespace {

// 24 点脉形（相对高度，px）：一个主峰 + 一个次峰，贴近腕部 PPG 的定性形状。
constexpr uint8_t kPulseShape[24] = {8,  10, 14, 20, 28, 38, 52, 44, 30, 22, 16, 12,
                                     9,  8,  9,  12, 16, 24, 34, 26, 18, 13, 10, 8};

constexpr int kWaveBaselineY = 330;
constexpr int kWaveStep = 12;

}  // namespace

void VitalScreen::onCreate(lv_obj_t* parent) {
    lv_obj_t* card = ui::makeCard(parent, 24, 72, 752, 368);

    ui::makeLabel(card, "心率", ui::bodyFont(), ui::kColorTextMuted, 40, 28);
    m_hr = ui::makeValue(card, ui::clockFont(), ui::kColorHeart, 40, 58);
    ui::makeLabel(card, "次/分", ui::bodyFont(), ui::kColorTextMuted, 236, 128);

    ui::makeLabel(card, "血氧 SpO2", ui::bodyFont(), ui::kColorTextMuted, 460, 28);
    m_spo2 = ui::makeValue(card, ui::numberFont(), ui::kColorSport, 460, 64);
    ui::makeLabel(card, "%", ui::bodyFont(), ui::kColorTextMuted, 540, 90);

    ui::makeLabel(card, "信号质量", ui::bodyFont(), ui::kColorTextMuted, 460, 150);
    for (int i = 0; i < 3; ++i) {
        m_quality[i] = ui::makeBar(card, 460 + i * 92, 186, 80, 16, ui::kColorCardBorder);
    }

    ui::makeLabel(card, "PPG 波形", ui::bodyFont(), ui::kColorTextMuted, 40, 220);
    for (int i = 0; i < kWaveCount; ++i) {
        const int height = kPulseShape[i];
        m_wave[i] = ui::makeBar(card, 40 + i * kWaveStep, kWaveBaselineY - height, 4, height,
                                ui::kColorHeart);
    }

    m_disabled = ui::makeLabel(card, "未启用", ui::titleFont(), ui::kColorTextMuted, 330, 116);
    ui::setVisible(m_disabled, false);
}

void VitalScreen::onEvent(const hb::Event& event) { (void)event; }

void VitalScreen::refresh(const ui::Model& model) {
    char buffer[32];
    if (ui::fmtHr(model, buffer, sizeof(buffer))) {
        ui::setText(m_hr, buffer);
    }
    if (ui::fmtSpo2(model, buffer, sizeof(buffer))) {
        ui::setText(m_spo2, buffer);
    }

    // 信号质量：有页面数据即亮一格，心率/血氧各自有效再加一格。
    int bars = 0;
    if (model.hrPageEnabled) {
        bars = 1;
        if (model.hrValid) {
            ++bars;
        }
        if (model.spo2Valid) {
            ++bars;
        }
    }
    for (int i = 0; i < 3; ++i) {
        ui::setColor(m_quality[i], i < bars ? ui::kColorSport : ui::kColorCardBorder);
    }

    ui::setVisible(m_disabled, !model.hrPageEnabled);
    // 未启用时波形整体压暗，避免被误读成实时数据。
    for (int i = 0; i < kWaveCount; ++i) {
        ui::setColor(m_wave[i], model.hrPageEnabled ? ui::kColorHeart : ui::kColorCardBorder);
    }
}

}  // namespace app
