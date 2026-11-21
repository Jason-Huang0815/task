#pragma once

#include "common/events.h"
#include "lvgl.h"
#include "ui/model.h"
#include "ui/nav_model.h"

// 页面基类：一个页面 = 一块独立 screen 对象（lv_obj_create(nullptr)），
// 创建后常驻，切屏只做装载与淡入，不做销毁重建。

namespace hb::ui {

class Screen {
public:
    virtual ~Screen() = default;

    virtual PageId id() const = 0;
    virtual const char* title() const = 0;

    /// 在 m_screen 上建界面；parent 就是 m_screen。
    virtual void onCreate(lv_obj_t* parent) = 0;

    virtual void onEnter() {}
    virtual void onLeave() {}

    /// 活动屏才会收到事件（按键、传感器事件都由 UiService 转发）。
    virtual void onEvent(const hb::Event& event) { (void)event; }

    /// 活动屏每秒刷新一次。
    virtual void refresh(const Model& model) { (void)model; }

    lv_obj_t* screen() const { return m_screen; }

    /// 由 ScreenManager::add 调用一次：建 screen 对象并交给 onCreate。
    /// 重复调用是空操作，便于宿主重复注册同一页面。
    void createScreen();

protected:
    lv_obj_t* m_screen = nullptr;
};

}  // namespace hb::ui
