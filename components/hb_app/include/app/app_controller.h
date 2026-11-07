#pragma once

#include "common/settings.h"
#include "service/event_bus.h"
#include "service/sedentary_service.h"
#include "service/step_service.h"

namespace hb {

/// 外观类：统一初始化、事件路由与 tick。
/// 对应文档里的 HealthBand；页面栈在 P4 接入。
class AppController {
public:
    explicit AppController(EventBus& bus);

    bool begin();
    void tick();

    StepService& stepService() { return m_step; }
    SedentaryService& sedentaryService() { return m_sedentary; }

private:
    EventBus& m_bus;
    Settings m_settings;
    StepService m_step;
    SedentaryService m_sedentary;
};

}  // namespace hb
