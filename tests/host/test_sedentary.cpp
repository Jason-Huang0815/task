#include "framework.h"
#include "service/sedentary_service.h"

using namespace hb;

namespace {
constexpr int64_t kMinuteUs = 60LL * 1000000LL;

void tickMinutes(SedentaryService& service, int64_t& nowUs, int minutes) {
    for (int i = 0; i < minutes; ++i) {
        nowUs += kMinuteUs;
        service.onMinuteTick(nowUs);
    }
}
}  // namespace

TEST_CASE(sedentary_alerts_after_threshold) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    int alerts = 0;
    bus.subscribe(EventType::Alert, [&](const Event&) { ++alerts; });

    service.setWorn(true);
    int64_t now = 0;
    tickMinutes(service, now, 44);
    CHECK(service.state() != SitState::Alerting);

    tickMinutes(service, now, 1);
    CHECK(service.state() == SitState::Alerting);
    CHECK_EQ(alerts, 1);
    CHECK_EQ(service.sittingMinutes(), 45);
}

TEST_CASE(sedentary_snooze_then_alert_again) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    service.setWorn(true);
    int64_t now = 0;
    tickMinutes(service, now, 45);
    CHECK(service.state() == SitState::Alerting);

    service.onSnooze(now);
    CHECK(service.state() == SitState::Snoozed);
    CHECK_EQ(service.snoozeCount(), 1);

    tickMinutes(service, now, 9);
    CHECK(service.state() == SitState::Snoozed);

    tickMinutes(service, now, 1);
    CHECK(service.state() == SitState::Alerting);
}

TEST_CASE(sedentary_30_steps_resets_episode) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    service.setWorn(true);
    int64_t now = 0;
    tickMinutes(service, now, 45);
    CHECK(service.state() == SitState::Alerting);

    service.onStep(30, now);
    CHECK(service.state() == SitState::Active);
    CHECK_EQ(service.sittingMinutes(), 0);
}

TEST_CASE(sedentary_snooze_is_capped) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    service.setWorn(true);
    int64_t now = 0;
    tickMinutes(service, now, 45);

    for (int i = 0; i < settings.maxSnooze; ++i) {
        service.onSnooze(now);
        CHECK(service.state() == SitState::Snoozed);
        tickMinutes(service, now, 10);
        CHECK(service.state() == SitState::Alerting);
    }

    const int count = service.snoozeCount();
    CHECK_EQ(count, settings.maxSnooze);

    service.onSnooze(now);
    CHECK(service.state() == SitState::Alerting);
    CHECK_EQ(service.snoozeCount(), count);
}

TEST_CASE(sedentary_quiet_keeps_timing_without_alert) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    int alerts = 0;
    bus.subscribe(EventType::Alert, [&](const Event&) { ++alerts; });

    service.setWorn(true);
    service.setQuiet(true);

    int64_t now = 0;
    tickMinutes(service, now, 60);
    CHECK(service.state() == SitState::Quiet);
    CHECK_EQ(alerts, 0);
    CHECK(service.sittingMinutes() >= 45);

    service.setQuiet(false);
    CHECK(service.state() == SitState::Active);
    tickMinutes(service, now, 1);
    CHECK(service.state() == SitState::Alerting);
    CHECK_EQ(alerts, 1);
}

TEST_CASE(sedentary_wear_off_resets) {
    EventBus bus;
    Settings settings;
    SedentaryService service(bus, settings);

    service.setWorn(true);
    int64_t now = 0;
    tickMinutes(service, now, 10);
    CHECK(service.sittingMinutes() > 0);

    service.setWorn(false);
    CHECK(service.state() == SitState::WearOff);
    CHECK_EQ(service.sittingMinutes(), 0);
}
