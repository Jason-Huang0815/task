#include "app/screens/watch_face.h"

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

// hb_ui 的公共类型在 hb::ui 里；应用层用短别名，写页面代码更省字符。
namespace ui = hb::ui;

// 表盘页实现：左 420×360 时间卡，右侧三张 316×112 数据卡（x = 468）。

namespace app {

void WatchFace::onCreate(lv_obj_t* parent) {
    lv_obj_t* timeCard = ui::makeCard(parent, 24, 72, 420, 360);
    ui::makeLabel(timeCard, "现在", ui::bodyFont(), ui::kColorTextMuted, 40, 34);
    m_time = ui::makeValue(timeCard, ui::clockFont(), ui::kColorText, 40, 66);
    m_date = ui::makeValue(timeCard, ui::titleFont(), ui::kColorText, 40, 214);
    m_weekday = ui::makeValue(timeCard, ui::bodyFont(), ui::kColorTextMuted, 40, 254);

    lv_obj_t* stepsCard = ui::makeCard(parent, 468, 72, 316, 112);
    ui::makeLabel(stepsCard, "步数", ui::bodyFont(), ui::kColorTextMuted, 20, 14);
    m_steps = ui::makeValue(stepsCard, ui::numberFont(), ui::kColorSport, 20, 40);
    ui::makeLabel(stepsCard, "步", ui::bodyFont(), ui::kColorTextMuted, 196, 66);

    lv_obj_t* hrCard = ui::makeCard(parent, 468, 200, 316, 112);
    ui::makeLabel(hrCard, "心率", ui::bodyFont(), ui::kColorTextMuted, 20, 14);
    m_hr = ui::makeValue(hrCard, ui::numberFont(), ui::kColorHeart, 20, 40);
    ui::makeLabel(hrCard, "次/分", ui::bodyFont(), ui::kColorTextMuted, 130, 66);

    lv_obj_t* tempCard = ui::makeCard(parent, 468, 328, 316, 112);
    ui::makeLabel(tempCard, "体温", ui::bodyFont(), ui::kColorTextMuted, 20, 14);
    m_temp = ui::makeValue(tempCard, ui::numberFont(), ui::kColorTemp, 20, 40);
    ui::makeLabel(tempCard, "℃", ui::bodyFont(), ui::kColorTextMuted, 150, 66);
}

void WatchFace::onEvent(const hb::Event& event) { (void)event; }

void WatchFace::refresh(const ui::Model& model) {
    char buffer[32];
    if (ui::fmtClock(model.epochSec, buffer, sizeof(buffer))) {
        ui::setText(m_time, buffer);
    }
    if (ui::fmtDate(model.epochSec, buffer, sizeof(buffer))) {
        ui::setText(m_date, buffer);
    }
    if (ui::fmtWeekday(model.epochSec, buffer, sizeof(buffer))) {
        ui::setText(m_weekday, buffer);
    }
    if (ui::fmtSteps(model.steps, buffer, sizeof(buffer))) {
        ui::setText(m_steps, buffer);
    }
    if (ui::fmtHr(model, buffer, sizeof(buffer))) {
        ui::setText(m_hr, buffer);
    }
    if (ui::fmtTemp(model, buffer, sizeof(buffer))) {
        ui::setText(m_temp, buffer);
    }
}

}  // namespace app
