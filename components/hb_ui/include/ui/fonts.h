#pragma once

#include "lvgl.h"

// 字体入口：页面只调用这里的访问器，不直接写 lv_font_* 名字。
// HB_UI_HAS_CN_FONT=1 时用 lv_font_conv 生成的中文子集（fonts/README.md），
// 否则退回 LVGL 内置字体，保证没有字体资源的仿真环境也能编译运行。

#ifndef HB_UI_HAS_CN_FONT
#define HB_UI_HAS_CN_FONT 0
#endif

#if HB_UI_HAS_CN_FONT
extern "C" {
// 由 tools/gen_font_subset.ps1 生成，源文件放在 components/hb_ui/fonts/ 下。
extern const lv_font_t hb_font_cn_16;
extern const lv_font_t hb_font_cn_20;
extern const lv_font_t hb_font_cn_26;
extern const lv_font_t hb_font_num_48;
extern const lv_font_t hb_font_num_112;
}
#endif

namespace hb::ui {

#if HB_UI_HAS_CN_FONT
/// 正文小字（16 px，含中文）。
inline const lv_font_t* textFont() { return &hb_font_cn_16; }
/// 正文（20 px，含中文）。
inline const lv_font_t* bodyFont() { return &hb_font_cn_20; }
/// 标题（26 px，含中文）。
inline const lv_font_t* titleFont() { return &hb_font_cn_26; }
/// 中等数字（48 px，等宽）。
inline const lv_font_t* numberFont() { return &hb_font_num_48; }
/// 表盘大数字（112 px，等宽）。
inline const lv_font_t* clockFont() { return &hb_font_num_112; }
#else
// 内置字体只有 Montserrat，中文会显示为空白；这是显式的降级路径，
// 由 README 里的字体生成步骤切换，不属于占位实现。
inline const lv_font_t* textFont() { return LV_FONT_DEFAULT; }
inline const lv_font_t* bodyFont() { return LV_FONT_DEFAULT; }
inline const lv_font_t* titleFont() { return LV_FONT_DEFAULT; }
inline const lv_font_t* numberFont() { return LV_FONT_DEFAULT; }
inline const lv_font_t* clockFont() { return LV_FONT_DEFAULT; }
#endif

}  // namespace hb::ui
