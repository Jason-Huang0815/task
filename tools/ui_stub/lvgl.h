#pragma once

// 仅供主机语法自检用的极简 LVGL 9 头文件桩：
// 只声明 ui_skeleton 实际用到的那批 API，签名与 LVGL 9.2 一致。

#include <cstddef>
#include <cstdint>

typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_event_t lv_event_t;
typedef struct _lv_indev_t lv_indev_t;
typedef struct _lv_display_t lv_display_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;

typedef struct {
    const void* glyph_bitmap;
} lv_font_t;

typedef uint32_t lv_style_selector_t;

typedef enum {
    LV_PART_MAIN = 0x000000,
    LV_PART_KNOB = 0x002000,
} lv_part_t;

typedef uint8_t lv_opa_t;

#define LV_OPA_TRANSP 0
#define LV_OPA_COVER 255

typedef enum {
    LV_ALIGN_TOP_LEFT = 0,
    LV_ALIGN_TOP_MID,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_CENTER,
} lv_align_t;

typedef enum {
    LV_TEXT_ALIGN_LEFT = 0,
    LV_TEXT_ALIGN_CENTER,
    LV_TEXT_ALIGN_RIGHT,
} lv_text_align_t;

typedef enum {
    LV_LABEL_LONG_WRAP = 0,
    LV_LABEL_LONG_CLIP,
} lv_label_long_mode_t;

typedef enum {
    LV_OBJ_FLAG_HIDDEN = (1 << 0),
    LV_OBJ_FLAG_CLICKABLE = (1 << 1),
    LV_OBJ_FLAG_SCROLLABLE = (1 << 2),
} lv_obj_flag_t;

typedef enum {
    LV_EVENT_PRESSED = 0,
    LV_EVENT_CLICKED = 7,
    LV_EVENT_GESTURE = 12,
    LV_EVENT_VALUE_CHANGED = 21,
} lv_event_code_t;

typedef enum {
    LV_SCR_LOAD_ANIM_NONE = 0,
    LV_SCR_LOAD_ANIM_FADE_ON = 3,
} lv_scr_load_anim_t;

typedef void (*lv_event_cb_t)(lv_event_t* event);

extern "C" {
const lv_font_t* lv_font_default(void);
}

#define LV_FONT_DEFAULT (lv_font_default())

lv_obj_t* lv_obj_create(lv_obj_t* parent);
void lv_obj_del(lv_obj_t* obj);
void lv_obj_set_size(lv_obj_t* obj, int32_t width, int32_t height);
void lv_obj_set_width(lv_obj_t* obj, int32_t width);
void lv_obj_set_height(lv_obj_t* obj, int32_t height);
void lv_obj_set_pos(lv_obj_t* obj, int32_t x, int32_t y);
void lv_obj_set_x(lv_obj_t* obj, int32_t x);
void lv_obj_set_y(lv_obj_t* obj, int32_t y);
void lv_obj_align(lv_obj_t* obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
void lv_obj_add_flag(lv_obj_t* obj, lv_obj_flag_t flag);
void lv_obj_remove_flag(lv_obj_t* obj, lv_obj_flag_t flag);
void lv_obj_add_event_cb(lv_obj_t* obj, lv_event_cb_t cb, lv_event_code_t filter, void* user_data);
void lv_obj_remove_style(lv_obj_t* obj, void* style, lv_style_selector_t selector);
void lv_obj_set_style_bg_color(lv_obj_t* obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t* obj, lv_opa_t value, lv_style_selector_t selector);
void lv_obj_set_style_radius(lv_obj_t* obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_border_width(lv_obj_t* obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_border_color(lv_obj_t* obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_pad_all(lv_obj_t* obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_color(lv_obj_t* obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t* obj, const lv_font_t* value, lv_style_selector_t selector);
void lv_obj_set_style_text_align(lv_obj_t* obj, lv_text_align_t value, lv_style_selector_t selector);
void lv_obj_set_style_arc_width(lv_obj_t* obj, int32_t value, lv_style_selector_t selector);
void lv_obj_set_style_arc_color(lv_obj_t* obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_opa(lv_obj_t* obj, lv_opa_t value, lv_style_selector_t selector);
void lv_obj_set_style_min_height(lv_obj_t* obj, int32_t value, lv_style_selector_t selector);

lv_color_t lv_color_hex(uint32_t color);

lv_obj_t* lv_label_create(lv_obj_t* parent);
void lv_label_set_text(lv_obj_t* obj, const char* text);
void lv_label_set_long_mode(lv_obj_t* obj, lv_label_long_mode_t mode);

lv_obj_t* lv_button_create(lv_obj_t* parent);

lv_obj_t* lv_arc_create(lv_obj_t* parent);
void lv_arc_set_range(lv_obj_t* obj, int32_t min, int32_t max);
void lv_arc_set_value(lv_obj_t* obj, int32_t value);
void lv_arc_set_bg_angles(lv_obj_t* obj, uint32_t start, uint32_t end);
void lv_arc_set_rotation(lv_obj_t* obj, uint32_t rotation);
void lv_arc_set_bg_start_angle(lv_obj_t* obj, uint32_t start);
void lv_arc_set_bg_end_angle(lv_obj_t* obj, uint32_t end);

lv_obj_t* lv_event_get_target(lv_event_t* event);
void* lv_event_get_user_data(lv_event_t* event);
lv_event_code_t lv_event_get_code(lv_event_t* event);

lv_obj_t* lv_screen_active(void);
void lv_screen_load(lv_obj_t* screen);
void lv_screen_load_anim(lv_obj_t* screen, lv_scr_load_anim_t anim_type, uint32_t time,
                         uint32_t delay, bool auto_del);

uint32_t lv_tick_get(void);
