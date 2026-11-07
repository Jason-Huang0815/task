#include "service/sedentary_service.h"

namespace hb {
namespace {
constexpr int64_t kMinuteUs = 60LL * 1000000LL;
}  // namespace

SedentaryService::SedentaryService(EventBus& bus, const Settings& settings)
    : m_bus(bus), m_cfg(settings) {}

void SedentaryService::setWorn(bool worn) {
    if (m_worn == worn) {
        return;
    }
    m_worn = worn;
    if (!worn) {
        resetEpisode();
        transition(SitState::WearOff, "worn off");
    } else if (m_quiet) {
        transition(SitState::Quiet, "quiet");
    } else {
        transition(SitState::Active, "worn");
    }
}

void SedentaryService::setQuiet(bool quiet) {
    if (m_quiet == quiet) {
        return;
    }
    m_quiet = quiet;
    if (!m_worn) {
        return;
    }
    transition(quiet ? SitState::Quiet : SitState::Active, quiet ? "quiet on" : "quiet off");
}

void SedentaryService::onMinuteTick(int64_t nowUs) {
    if (!m_worn) {
        return;
    }

    if (m_quiet) {
        if (m_state != SitState::Quiet) {
            transition(SitState::Quiet, "quiet");
        }
        advanceSitting(nowUs);  // 免打扰只暂停提醒，不暂停计时
        return;
    }

    if (m_state == SitState::Quiet) {
        transition(SitState::Active, "quiet end");
    }
    if (m_state == SitState::WearOff) {
        transition(SitState::Active, "worn");
    }

    switch (m_state) {
        case SitState::Active:
        case SitState::Sitting: {
            if (stepsInLast5Min(nowUs) >= m_cfg.stepsPer5Min) {
                m_sittingMin = 0;
                if (m_state == SitState::Sitting) {
                    transition(SitState::Active, "walked");
                }
                break;
            }
            advanceSitting(nowUs);
            if (m_sittingMin >= m_cfg.thresholdMin) {
                transition(SitState::Alerting, "threshold");
            } else if (m_state == SitState::Active) {
                transition(SitState::Sitting, "counting");
            }
            break;
        }
        case SitState::Alerting:
            break;  // 等用户操作或 60 s 超时
        case SitState::Snoozed:
            if (nowUs >= m_snoozeUntilUs) {
                transition(SitState::Alerting, "snooze done");
            }
            break;
        case SitState::Quiet:
        case SitState::WearOff:
            break;
    }
}

void SedentaryService::onStep(int steps, int64_t nowUs) {
    if (steps <= 0) {
        return;
    }
    addSteps(steps, nowUs);
    if (!m_worn) {
        return;
    }
    if (stepsInLast5Min(nowUs) >= m_cfg.stepsPer5Min) {
        m_sittingMin = 0;
        if (m_state == SitState::Sitting || m_state == SitState::Alerting ||
            m_state == SitState::Snoozed) {
            m_snoozeCount = 0;
            transition(SitState::Active, "walked 30 steps");
        }
    }
}

void SedentaryService::onUserStandUp(int64_t nowUs) {
    (void)nowUs;
    if (m_state != SitState::Alerting) {
        return;
    }
    m_sittingMin = 0;
    m_snoozeCount = 0;
    transition(SitState::Active, "user stand up");
}

void SedentaryService::onSnooze(int64_t nowUs) {
    if (m_state != SitState::Alerting || m_snoozeCount >= m_cfg.maxSnooze) {
        return;
    }
    ++m_snoozeCount;
    m_snoozeUntilUs = nowUs + static_cast<int64_t>(m_cfg.snoozeMin) * kMinuteUs;
    transition(SitState::Snoozed, "snooze");
}

void SedentaryService::onAlertTimeout(int64_t nowUs) {
    onSnooze(nowUs);
}

void SedentaryService::transition(SitState next, const char* reason) {
    (void)reason;  // 预留：P3 接入日志/UI 事件
    if (m_state == next) {
        return;
    }
    m_state = next;
    if (next == SitState::Alerting) {
        m_bus.publish({EventType::Alert, m_sittingMin, AlertKind::Sedentary});
    }
}

void SedentaryService::resetEpisode() {
    m_sittingMin = 0;
    m_snoozeCount = 0;
    m_snoozeUntilUs = 0;
    for (auto& bucket : m_buckets) {
        bucket = Bucket{};
    }
}

void SedentaryService::addSteps(int steps, int64_t nowUs) {
    const int64_t minute = nowUs / kMinuteUs;
    Bucket& bucket = m_buckets[static_cast<size_t>(minute % 5)];
    if (bucket.minute != minute) {
        bucket.minute = minute;
        bucket.steps = 0;
    }
    const int sum = static_cast<int>(bucket.steps) + steps;
    bucket.steps = static_cast<uint16_t>(sum > 65535 ? 65535 : sum);
}

int SedentaryService::stepsInLast5Min(int64_t nowUs) const {
    const int64_t minute = nowUs / kMinuteUs;
    int sum = 0;
    for (const auto& bucket : m_buckets) {
        if (bucket.minute >= minute - 4 && bucket.minute <= minute) {
            sum += bucket.steps;
        }
    }
    return sum;
}

void SedentaryService::advanceSitting(int64_t nowUs) {
    if (stepsInLast5Min(nowUs) >= m_cfg.stepsPer5Min) {
        m_sittingMin = 0;
        return;
    }
    ++m_sittingMin;
}

}  // namespace hb
