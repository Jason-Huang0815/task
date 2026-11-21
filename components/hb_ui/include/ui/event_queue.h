#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "common/events.h"

// 跨线程事件缓冲：传感器/按键任务把事件投进来，ui_task 在自己的循环里排空。
// 满时丢最旧的一条并计数——界面宁可丢一个 Step，也不能阻塞事件源。

namespace hb::ui {

class EventQueue {
public:
    static constexpr int kCapacity = 32;

    /// 入队。队列已满时丢弃最旧的一条并返回 false（事件本身仍然入队）。
    bool push(const hb::Event& event);

    /// 出队；无事件返回 false。
    bool pop(hb::Event& out);

    size_t size() const;
    uint32_t dropped() const;

    EventQueue() = default;
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

private:
    mutable std::mutex m_mutex;
    hb::Event m_items[kCapacity];
    int m_head = 0;  // 下一个要读的位置
    int m_tail = 0;  // 下一个要写的位置
    int m_count = 0;
    uint32_t m_dropped = 0;
};

}  // namespace hb::ui
