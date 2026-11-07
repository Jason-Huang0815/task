#pragma once

#include <cstdint>

#include "common/settings.h"
#include "common/types.h"
#include "service/event_bus.h"
#include "service/ring_window.h"

namespace hb {

/// 计步服务：SVM → 一阶低通 → 5 s 滑窗动态阈值 → 峰谷对 → 步事件。
///
/// 线程模型：目前由 imu_task 同步调用；P2 改为队列消费。
class StepService {
public:
    explicit StepService(EventBus& bus, const Settings& settings);

    void onImuFrame(const ImuFrame& frame);
    void reset();

    /// 马达震动期间及之后屏蔽计步（默认 300 ms）。
    void muteFor(int64_t nowUs, int64_t durationUs);

    uint32_t steps() const { return m_steps; }
    float cadence() const { return m_cadence; }
    bool isMuted(int64_t nowUs) const { return nowUs < m_muteUntilUs; }

private:
    EventBus& m_bus;
    Settings m_cfg;

    RingWindow<250> m_window;  // 5 s × 50 Hz
    float m_lp = 0.0f;
    bool m_initialized = false;

    bool m_armed = false;
    float m_peak = 0.0f;
    float m_valley = 0.0f;

    uint32_t m_steps = 0;
    float m_cadence = 0.0f;
    int64_t m_lastStepUs = 0;
    int64_t m_muteUntilUs = 0;
};

}  // namespace hb
