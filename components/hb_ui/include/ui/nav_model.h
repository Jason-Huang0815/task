#pragma once

#include <cstdint>

// 纯逻辑页面导航：不碰 LVGL，可在主机上单测。
// 环形顺序 表盘 -> 运动 -> 心率 -> 体温 -> 久坐 -> 设置 -> 表盘；
// 弹窗打开时 setLocked(true)，此时 next/prev/home 全部无效。

namespace hb::ui {

enum class PageId : uint8_t {
    Watch = 0,
    Sport,
    Heart,
    Temp,
    Sedentary,
    Settings,
    Count,
};

constexpr int kPageCount = static_cast<int>(PageId::Count);

class NavModel {
public:
    void next();
    void prev();
    void home();

    PageId current() const { return at(m_index); }
    int index() const { return m_index; }

    void setLocked(bool locked) { m_locked = locked; }
    bool locked() const { return m_locked; }

    /// PageId -> 槽位下标；非法值按 PageId::Count 取模。
    static int indexOf(PageId id) { return static_cast<int>(id); }

    /// 槽位下标 -> PageId，负数与越界都回绕，保证永远返回合法页。
    static PageId at(int index) {
        int slot = index % kPageCount;
        if (slot < 0) {
            slot += kPageCount;
        }
        return static_cast<PageId>(slot);
    }

private:
    int m_index = 0;
    bool m_locked = false;
};

}  // namespace hb::ui
