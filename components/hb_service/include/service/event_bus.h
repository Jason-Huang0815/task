#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "common/events.h"

namespace hb {

/// 轻量同步事件总线。
///
/// 约束：
/// - publish() 在调用者线程直接执行订阅者，不做队列、不分配内存；
/// - 订阅者必须快速返回，禁止在回调里 subscribe()/unsubscribe()（会死锁）；
/// - 界面应把事件投递到 UI 队列，而不是在回调里直接操作 LVGL。
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    /// 返回订阅 id，用于退订。
    int subscribe(EventType type, Handler handler);
    void unsubscribe(int id);
    void publish(const Event& event);

    size_t subscriberCount() const;

private:
    struct Subscription {
        int id;
        EventType type;
        Handler handler;
    };

    mutable std::mutex m_mutex;
    std::vector<Subscription> m_subs;
    int m_nextId = 0;
};

}  // namespace hb
