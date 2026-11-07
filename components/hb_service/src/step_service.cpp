#include "service/step_service.h"

#include <algorithm>
#include <cmath>

namespace hb {
namespace {
constexpr float kLowPassAlpha = 0.2f;
constexpr float kThresholdSigma = 0.8f;
constexpr float kReleaseSigma = 0.2f;
constexpr float kMinStdDev = 0.02f;   // 静止时避免噪声触发
constexpr int64_t kMinStepUs = 250000;    // 最快 4 步/秒
constexpr int64_t kMaxStepUs = 2000000;   // 最慢 0.5 步/秒
}  // namespace

StepService::StepService(EventBus& bus, const Settings& settings) : m_bus(bus), m_cfg(settings) {}

void StepService::onImuFrame(const ImuFrame& frame) {
    const float svm =
        std::sqrt(frame.ax * frame.ax + frame.ay * frame.ay + frame.az * frame.az);

    if (!m_initialized) {
        m_lp = svm;
        m_initialized = true;
        m_valley = m_lp;
    } else {
        m_lp += kLowPassAlpha * (svm - m_lp);
    }
    m_window.push(m_lp);

    const int64_t nowUs = frame.timestampUs;
    if (isMuted(nowUs)) {
        m_armed = false;
        return;
    }

    const float mean = m_window.mean();
    const float sd = m_window.stddev();
    if (sd < kMinStdDev) {
        m_armed = false;
        return;
    }

    const float threshold = mean + kThresholdSigma * sd;
    const float release = threshold - kReleaseSigma * sd;

    if (!m_armed) {
        // 未起峰时持续跟踪波谷，作为本次峰值的参考基线
        m_valley = std::min(m_valley, m_lp);
        if (m_lp > threshold) {
            m_armed = true;
            m_peak = m_lp;
        }
        return;
    }

    m_peak = std::max(m_peak, m_lp);
    if (m_lp >= release) {
        return;
    }

    // 一个完整峰谷对：回落即确认峰值，幅度用“峰 - 起峰前的谷”
    const bool firstStep = (m_lastStepUs == 0);
    const int64_t deltaUs = firstStep ? 0 : (nowUs - m_lastStepUs);
    const bool intervalOk = firstStep || deltaUs >= kMinStepUs;

    if ((m_peak - m_valley) >= m_cfg.stepPeakG && intervalOk) {
        ++m_steps;
        // 间隔过长（例如中途停下）仍算一步，但步频归零
        m_cadence = (firstStep || deltaUs > kMaxStepUs)
                        ? 0.0f
                        : 60000000.0f / static_cast<float>(deltaUs);
        m_lastStepUs = nowUs;
        m_bus.publish({EventType::Step, 1});
    }

    m_armed = false;
    m_valley = m_lp;
}

void StepService::reset() {
    m_window.clear();
    m_lp = 0.0f;
    m_initialized = false;
    m_armed = false;
    m_peak = 0.0f;
    m_valley = 0.0f;
    m_steps = 0;
    m_cadence = 0.0f;
    m_lastStepUs = 0;
    m_muteUntilUs = 0;
}

void StepService::muteFor(int64_t nowUs, int64_t durationUs) {
    const int64_t until = nowUs + durationUs;
    if (until > m_muteUntilUs) {
        m_muteUntilUs = until;
    }
    m_armed = false;
}

}  // namespace hb
