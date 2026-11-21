#include "ui/screen_manager.h"

#include "ui/lvgl_compat.h"
#include "ui/theme.h"

// 页面容器实现：Screen::createScreen 也放在这里，因为"建 screen 对象"和"装载 screen"
// 是同一条生命周期，分到两个文件反而更难读。

namespace hb::ui {

void Screen::createScreen() {
    if (m_screen != nullptr) {
        return;
    }
    m_screen = lv_obj_create(nullptr);
    if (m_screen == nullptr) {
        return;
    }
    lv_obj_remove_style(m_screen, nullptr, 0);
    lv_obj_set_size(m_screen, kScreenWidth, kScreenHeight);
    lv_obj_set_style_bg_color(m_screen, lv_color_hex(kColorBg), 0);
    lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_screen, 0, 0);
    lv_obj_set_style_pad_all(m_screen, 0, 0);
    lv_obj_remove_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
    onCreate(m_screen);
}

void ScreenManager::begin() { m_active = -1; }

bool ScreenManager::add(Screen* screen) {
    if (screen == nullptr) {
        return false;
    }
    const int index = NavModel::indexOf(screen->id());
    if (index < 0 || index >= kPageCount) {
        return false;
    }
    if (m_screens[index] != nullptr) {
        return false;  // 同一页只允许注册一次，避免两个 screen 对象争用一个槽位
    }
    m_screens[index] = screen;
    screen->createScreen();
    return true;
}

bool ScreenManager::has(PageId id) const {
    const int index = NavModel::indexOf(id);
    return index >= 0 && index < kPageCount && m_screens[index] != nullptr &&
           m_screens[index]->screen() != nullptr;
}

void ScreenManager::show(PageId id, bool animate) {
    const int index = NavModel::indexOf(id);
    if (index < 0 || index >= kPageCount) {
        return;
    }
    Screen* target = m_screens[index];
    if (target == nullptr || target->screen() == nullptr) {
        return;  // 目标页没注册：保持当前页不动
    }
    if (m_active == index) {
        return;
    }
    if (m_active >= 0 && m_screens[m_active] != nullptr) {
        m_screens[m_active]->onLeave();
    }
    compat::loadScreen(target->screen(), animate);
    m_active = index;
    target->onEnter();
}

void ScreenManager::refresh(const Model& model) {
    if (m_active < 0 || m_active >= kPageCount) {
        return;
    }
    Screen* screen = m_screens[m_active];
    if (screen != nullptr) {
        screen->refresh(model);
    }
}

PageId ScreenManager::active() const {
    if (m_active < 0 || m_active >= kPageCount) {
        return PageId::Watch;
    }
    return m_screens[m_active] != nullptr ? m_screens[m_active]->id() : PageId::Watch;
}

Screen* ScreenManager::activeScreen() {
    if (m_active < 0 || m_active >= kPageCount) {
        return nullptr;
    }
    return m_screens[m_active];
}

}  // namespace hb::ui
