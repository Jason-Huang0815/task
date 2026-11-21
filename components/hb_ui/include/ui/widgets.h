#pragma once

#include "lvgl.h"
#include "ui/theme.h"

// 通用控件工厂：卡片、文字、按钮、圆弧、竖条。
// 这里只使用 LVGL 9 的稳定 API，样式一律用 lv_obj_set_style_* 逐项设置，
// 不定义全局 lv_style_t，避免把主题耦合进页面。

namespace hb::ui {

/// 圆角卡片：底色 kColorCard、1px 边框 kColorCardBorder、无内边距。
lv_obj_t* makeCard(lv_obj_t* parent, int x, int y, int w, int h);

/// 普通文字（自带空指针保护，text 为空时写成空串）。
lv_obj_t* makeLabel(lv_obj_t* parent, const char* text, const lv_font_t* font, uint32_t color,
                    int x, int y);

/// 数值文字：先建空标签，数值由 refresh() 每秒写入。
lv_obj_t* makeValue(lv_obj_t* parent, const lv_font_t* font, uint32_t color, int x, int y);

/// 按钮：圆角 kButtonRadiusPx、无边框、文字居中。
/// textColor 默认深色（配青绿/琥珀等亮底），深色底按钮请显式传 kColorText。
lv_obj_t* makeButton(lv_obj_t* parent, int x, int y, int w, int h, const char* text,
                     uint32_t bgColor, uint32_t textColor = kColorBg);

/// 270° 目标环：去掉 knob，轨道色 trackColor，进度色 color，取值范围 0–100。
lv_obj_t* makeArc(lv_obj_t* parent, int x, int y, int size, int thickness, uint32_t color,
                  uint32_t trackColor);

/// 竖条（柱状图/波形/趋势图的基本单元），圆角 w/2。
lv_obj_t* makeBar(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color);

/// 以下三个都对空指针安全，页面 refresh() 里可以放心调用。
void setText(lv_obj_t* label, const char* text);
void setColor(lv_obj_t* obj, uint32_t color);
void setVisible(lv_obj_t* obj, bool visible);

/// 设置圆弧百分比，输入会夹到 0–100。
void setArcPercent(lv_obj_t* arc, int percent);

}  // namespace hb::ui
