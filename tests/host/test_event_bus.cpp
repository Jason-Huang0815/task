#include "framework.h"
#include "service/event_bus.h"

using namespace hb;

TEST_CASE(event_bus_dispatches_matching_type) {
    EventBus bus;
    int steps = 0;
    int alerts = 0;

    bus.subscribe(EventType::Step, [&](const Event&) { ++steps; });
    bus.subscribe(EventType::Alert, [&](const Event&) { ++alerts; });

    bus.publish({EventType::Step, 1});
    bus.publish({EventType::Alert, 0, AlertKind::Sedentary});

    CHECK_EQ(steps, 1);
    CHECK_EQ(alerts, 1);
}

TEST_CASE(event_bus_unsubscribe_stops_delivery) {
    EventBus bus;
    int count = 0;
    const int id = bus.subscribe(EventType::Step, [&](const Event&) { ++count; });

    bus.publish({EventType::Step, 1});
    CHECK_EQ(count, 1);

    bus.unsubscribe(id);
    bus.publish({EventType::Step, 1});

    CHECK_EQ(count, 1);
    CHECK_EQ(static_cast<int>(bus.subscriberCount()), 0);
}
