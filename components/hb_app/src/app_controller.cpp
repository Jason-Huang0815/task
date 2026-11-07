#include "app/app_controller.h"

#include "common/version.h"
#include "esp_log.h"

namespace hb {
namespace {
const char* kTag = "app";
}  // namespace

AppController::AppController(EventBus& bus)
    : m_bus(bus), m_step(bus, m_settings), m_sedentary(bus, m_settings) {}

bool AppController::begin() {
    // TODO(P1): I2CBus::instance().begin(GPIO_NUM_8, GPIO_NUM_9);
    // TODO(P1): 显示与触摸初始化
    // TODO(P2): 三颗传感器 begin()，失败时进入最小系统模式
    // TODO(P2): 创建 imu_task / ppg_task / ui_task ...
    ESP_LOGI(kTag, "skeleton ready, version %s", version());
    return true;
}

void AppController::tick() {
    // TODO(P2): 从队列取 ImuFrame，喂给 m_step.onImuFrame()
    // TODO(P3): 每分钟调用 m_sedentary.onMinuteTick()
    // TODO(P4): 把事件投递到 UI 队列
}

}  // namespace hb
