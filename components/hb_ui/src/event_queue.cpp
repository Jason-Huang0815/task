#include "ui/event_queue.h"

// 事件环形缓冲：push 在事件源任务里调用，pop 在 ui_task 里调用，互斥量保护。
// 满时覆盖最旧一条并累加 dropped，保证事件源永不被界面拖慢。

namespace hb::ui {

bool EventQueue::push(const hb::Event& event) {
    std::lock_guard<std::mutex> guard(m_mutex);
    bool keptAll = true;
    if (m_count == kCapacity) {
        m_head = (m_head + 1) % kCapacity;
        --m_count;
        ++m_dropped;
        keptAll = false;
    }
    m_items[m_tail] = event;
    m_tail = (m_tail + 1) % kCapacity;
    ++m_count;
    return keptAll;
}

bool EventQueue::pop(hb::Event& out) {
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_count == 0) {
        return false;
    }
    out = m_items[m_head];
    m_head = (m_head + 1) % kCapacity;
    --m_count;
    return true;
}

size_t EventQueue::size() const {
    std::lock_guard<std::mutex> guard(m_mutex);
    return static_cast<size_t>(m_count);
}

uint32_t EventQueue::dropped() const {
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_dropped;
}

}  // namespace hb::ui
