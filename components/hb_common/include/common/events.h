#pragma once

#include <cstdint>

namespace hb {

enum class EventType : uint8_t {
    ImuFrame = 0,
    PpgSample,
    Step,
    MinuteTick,
    Alert,
    UserStandUp,
    Snooze,
    AlertTimeout,
    KeyShort,
    KeyLong,
    KeyVeryLong,
    WearChanged,
};

enum class AlertKind : uint8_t {
    None = 0,
    Sedentary,
    Fall,
};

/// 事件总线里流动的最小消息。只传值，不传指针。
struct Event {
    EventType type;
    int32_t value = 0;
    AlertKind alert = AlertKind::None;
};

}  // namespace hb
