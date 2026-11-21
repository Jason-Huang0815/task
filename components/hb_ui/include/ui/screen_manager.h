#pragma once

#include "lvgl.h"
#include "ui/model.h"
#include "ui/nav_model.h"
#include "ui/screen.h"

// 页面集装箱：持有 kPageCount 个 Screen 指针，负责装载与 180ms 淡入。
// 页面对象在启动时创建一次，切屏不销毁；装载统一走 ui::compat::loadScreen。

namespace hb::ui {

class ScreenManager {
public:
    /// 复位内部状态（不销毁已注册页面）。
    void begin();

    /// 按 Screen::id() 放入槽位；空指针或重复注册返回 false。
    bool add(Screen* screen);

    /// 切到某页。目标未注册时保持原页不动；同一页重复调用无副作用。
    void show(PageId id, bool animate = true);

    /// 只刷新当前可见页。
    void refresh(const Model& model);

    /// 该页是否已注册（未注册时 show() 不会切屏，UiService 也不会重建状态栏）。
    bool has(PageId id) const;

    PageId active() const;
    Screen* activeScreen();

private:
    Screen* m_screens[kPageCount] = {nullptr};
    int m_active = -1;
};

}  // namespace hb::ui
