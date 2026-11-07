#pragma once

#include "common/settings.h"

namespace hb {

struct ActivityWindow {
    int sittingMinutes = 0;
    int stepsInLast5Min = 0;
};

/// 久坐判定策略（策略模式）。
/// 当前 SedentaryService 直接使用 Settings；P3 再把策略注入，便于演示模式。
class SedentaryPolicy {
public:
    virtual ~SedentaryPolicy() = default;
    virtual bool shouldAlert(const ActivityWindow& window, const Settings& settings) const = 0;
};

class StandardPolicy final : public SedentaryPolicy {
public:
    bool shouldAlert(const ActivityWindow& window, const Settings& settings) const override {
        return window.sittingMinutes >= settings.thresholdMin &&
               window.stepsInLast5Min < settings.stepsPer5Min;
    }
};

/// 课堂演示：60 s 触发，必须显式标注，不能悄悄改阈值。
class DemoPolicy final : public SedentaryPolicy {
public:
    bool shouldAlert(const ActivityWindow& window, const Settings& settings) const override {
        (void)settings;
        return window.sittingMinutes >= 1;
    }
};

}  // namespace hb
