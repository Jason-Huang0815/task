#include <cmath>

#include "framework.h"
#include "service/step_service.h"

using namespace hb;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kSampleRate = 50.0;

/// 合成一段步态：az 在 1 g 上下摆动，ax 带 90 度相位差。
void feedWalk(StepService& service, double seconds, double frequencyHz, int64_t startUs) {
    const int samples = static_cast<int>(seconds * kSampleRate);
    for (int i = 0; i < samples; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        ImuFrame frame;
        frame.az = static_cast<float>(1.0 + 0.9 * std::sin(2.0 * kPi * frequencyHz * t));
        frame.ax = static_cast<float>(0.3 * std::sin(2.0 * kPi * frequencyHz * t + 0.5));
        frame.timestampUs = startUs + static_cast<int64_t>(t * 1e6);
        service.onImuFrame(frame);
    }
}
}  // namespace

TEST_CASE(step_counts_synthetic_walk) {
    EventBus bus;
    Settings settings;
    StepService service(bus, settings);

    int stepEvents = 0;
    bus.subscribe(EventType::Step, [&](const Event&) { ++stepEvents; });

    feedWalk(service, 12.0, 1.8, 1000000);

    CHECK(service.steps() >= 10);
    CHECK(service.steps() <= 26);
    CHECK_EQ(stepEvents, static_cast<int>(service.steps()));
    CHECK(service.cadence() > 0.0f);
}

TEST_CASE(step_suppressed_while_muted) {
    EventBus bus;
    Settings settings;
    StepService service(bus, settings);

    service.muteFor(1000000, 60LL * 1000000LL);
    feedWalk(service, 10.0, 1.8, 1000000);
    CHECK_EQ(service.steps(), 0u);

    service.reset();
    feedWalk(service, 10.0, 1.8, 1000000);
    CHECK(service.steps() >= 8);
}

TEST_CASE(step_reset_zeroes_counters) {
    EventBus bus;
    Settings settings;
    StepService service(bus, settings);

    feedWalk(service, 10.0, 1.8, 1000000);
    CHECK(service.steps() > 0);

    service.reset();
    CHECK_EQ(service.steps(), 0u);
    CHECK_EQ(service.cadence(), 0.0f);
}
