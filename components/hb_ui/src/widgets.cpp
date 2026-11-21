#include "ui/widgets.h"

#include "ui/fonts.h"
#include "ui/theme.h"

// 控件工厂实现：所有对象都挂到 parent，坐标是 parent 内的绝对坐标。
// 样式一律逐项设置，不定义全局 lv_style_t，页面之间互不影响。

namespace hb::ui {
namespace {

/// 统一去掉主题默认样式，避免不同 app 主题下边框/内边距不一致。
void stripDefaultStyle(lv_obj_t* obj) {
    lv_obj_remove_style(obj, nullptr, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

}  // namespace

lv_obj_t* makeCard(lv_obj_t* parent, int x, int y, int w, int h) {
    if (parent == nullptr) {
        return nullptr;
    }
    lv_obj_t* card = lv_obj_create(parent);
    if (card == nullptr) {
        return nullptr;
    }
    stripDefaultStyle(card);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_radius(card, kCardRadiusPx, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColorCard), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(kColorCardBorder), 0);
    return card;
}

lv_obj_t* makeLabel(lv_obj_t* parent, const char* text, const lv_font_t* font, uint32_t color,
                    int x, int y) {
    if (parent == nullptr) {
        return nullptr;
    }
    lv_obj_t* label = lv_label_create(parent);
    if (label == nullptr) {
        return nullptr;
    }
    lv_obj_remove_style(label, nullptr, 0);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, text != nullptr ? text : "");
    lv_obj_set_style_text_font(label, font != nullptr ? font : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    return label;
}

lv_obj_t* makeValue(lv_obj_t* parent, const lv_font_t* font, uint32_t color, int x, int y) {
    return makeLabel(parent, "", font, color, x, y);
}

lv_obj_t* makeButton(lv_obj_t* parent, int x, int y, int w, int h, const char* text,
                     uint32_t bgColor, uint32_t textColor) {
    if (parent == nullptr) {
        return nullptr;
    }
    lv_obj_t* button = lv_button_create(parent);
    if (button == nullptr) {
        return nullptr;
    }
    stripDefaultStyle(button);
    lv_obj_set_size(button, w, h);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_radius(button, kButtonRadiusPx, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(bgColor), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);

    // 默认深色字配亮底；深色底按钮由调用方传 textColor = kColorText。
    lv_obj_t* label = makeLabel(button, text, bodyFont(), textColor, 0, (h - kBodyPx) / 2);
    if (label != nullptr) {
        lv_obj_set_width(label, w);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }
    return button;
}

lv_obj_t* makeArc(lv_obj_t* parent, int x, int y, int size, int thickness, uint32_t color,
                  uint32_t trackColor) {
    if (parent == nullptr) {
        return nullptr;
    }
    lv_obj_t* arc = lv_arc_create(parent);
    if (arc == nullptr) {
        return nullptr;
    }
    stripDefaultStyle(arc);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, x, y);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(arc, thickness, 0);    // 前景弧
    lv_obj_set_style_arc_color(arc, lv_color_hex(color), 0);
    lv_obj_set_style_arc_width(arc, thickness, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(trackColor), LV_PART_MAIN);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);  // 去掉可拖拽圆点
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_arc_set_bg_angles(arc, 0, 270);  // 270° 表盘，右下留缺口
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    return arc;
}

lv_obj_t* makeBar(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color) {
    if (parent == nullptr) {
        return nullptr;
    }
    lv_obj_t* bar = lv_obj_create(parent);
    if (bar == nullptr) {
        return nullptr;
    }
    stripDefaultStyle(bar);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_style_radius(bar, w > 0 ? w / 2 : 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    return bar;
}

void setText(lv_obj_t* label, const char* text) {
    if (label == nullptr) {
        return;
    }
    lv_label_set_text(label, text != nullptr ? text : "");
}

void setColor(lv_obj_t* obj, uint32_t color) {
    if (obj == nullptr) {
        return;
    }
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
}

void setVisible(lv_obj_t* obj, bool visible) {
    if (obj == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void setArcPercent(lv_obj_t* arc, int percent) {
    if (arc == nullptr) {
        return;
    }
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    lv_arc_set_value(arc, percent);
}

}  // namespace hb::ui
