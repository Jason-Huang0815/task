#include "ui/lvgl_compat.h"

#include "ui/theme.h"

// LVGL 9 兼容包装：唯一出现屏幕装载/删除 API 的地方。
// 升级到 LVGL 10 时只改这里，页面与 UiService 不受影响。

namespace hb::ui::compat {

void loadScreen(lv_obj_t* screen, bool animate) {
    if (screen == nullptr) {
        return;
    }
    if (animate) {
        lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_ON, kPageAnimMs, 0, false);
        return;
    }
    lv_screen_load(screen);
}

lv_obj_t* activeScreen() { return lv_screen_active(); }

void deleteObj(lv_obj_t* obj) {
    if (obj != nullptr) {
        lv_obj_del(obj);
    }
}

}  // namespace hb::ui::compat
