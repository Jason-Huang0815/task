#pragma once

#include <array>
#include <cstdint>

#include "common/events.h"
#include "common/settings.h"
#include "service/event_bus.h"

namespace hb {

enum class SitState : uint8_t {
    WearOff = 0,
    Active,
    Sitting,
    Alerting,
    Snoozed,
    Quiet,
};

/// 久坐状态机。6 个状态、单调时钟计时、免打扰与稍后提醒。
class SedentaryService {
public:
    explicit SedentaryService(EventBus& bus, const Settings& settings);

    void setWorn(bool worn);
    void setQuiet(bool quiet);

    /// 每分钟调用一次，驱动计时与状态迁移。
    void onMinuteTick(int64_t nowUs);

    /// 每来一步调用一次（或批量传入），用于统计 5 min 活动窗口。
    void onStep(int steps, int64_t nowUs);

    void onUserStandUp(int64_t nowUs);
    void onSnooze(int64_t nowUs);
    void onAlertTimeout(int64_t nowUs);  // 60 s 无操作，等同稍后

    SitState state() const { return m_state; }
    int sittingMinutes() const { return m_sittingMin; }
    int snoozeCount() const { return m_snoozeCount; }
    bool isAlerting() const { return m_state == SitState::Alerting; }

private:
    struct Bucket {
        int64_t minute = -1;
        uint16_t steps = 0;
    };

    void transition(SitState next, const char* reason);
    void resetEpisode();
    void addSteps(int steps, int64_t nowUs);
    int stepsInLast5Min(int64_t nowUs) const;
    void advanceSitting(int64_t nowUs);

    EventBus& m_bus;
    Settings m_cfg;

    SitState m_state = SitState::WearOff;
    bool m_worn = false;
    bool m_quiet = false;

    int m_sittingMin = 0;
    int m_snoozeCount = 0;
    int64_t m_snoozeUntilUs = 0;

    std::array<Bucket, 5> m_buckets{};
};

}  // namespace hb
