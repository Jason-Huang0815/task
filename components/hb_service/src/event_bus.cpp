#include "service/event_bus.h"

#include <utility>

namespace hb {

int EventBus::subscribe(EventType type, Handler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const int id = ++m_nextId;
    m_subs.push_back({id, type, std::move(handler)});
    return id;
}

void EventBus::unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_subs.begin(); it != m_subs.end(); ++it) {
        if (it->id == id) {
            m_subs.erase(it);
            return;
        }
    }
}

void EventBus::publish(const Event& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sub : m_subs) {
        if (sub.type == event.type) {
            sub.handler(event);
        }
    }
}

size_t EventBus::subscriberCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_subs.size();
}

}  // namespace hb
