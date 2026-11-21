#pragma once

#include "lvgl.h"

// LVGL 版本兼容层：v8 -> v9 的命名差异（scr_act/scr_load 等）只出现在这里，
// 其它源文件一律调用 compat::*，将来升级只需改这一个文件。

namespace hb::ui::compat {

/// 装载屏幕。animate=true 时 180ms 淡入（LV_SCR_LOAD_ANIM_FADE_ON），否则立即切换。
void loadScreen(lv_obj_t* screen, bool animate);

/// 当前活动屏。
lv_obj_t* activeScreen();

/// 删除对象（空指针安全）；删除入口集中在这里，便于统计与替换。
void deleteObj(lv_obj_t* obj);

}  // namespace hb::ui::compat
