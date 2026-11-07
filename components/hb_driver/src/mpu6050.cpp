#include "driver/mpu6050.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hb {
namespace {
const char* kTag = "mpu6050";
constexpr uint8_t kWhoAmI = 0x75;
constexpr uint8_t kConfig = 0x1A;
constexpr uint8_t kSampleRateDiv = 0x19;
constexpr uint8_t kGyroConfig = 0x1B;
constexpr uint8_t kAccelConfig = 0x1C;
constexpr uint8_t kPowerMgmt = 0x6B;
constexpr uint8_t kAccelXoutH = 0x3B;

inline int16_t toInt16(uint8_t hi, uint8_t lo) {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
}
}  // namespace

Mpu6050::Mpu6050(I2CBus& bus, uint8_t addr) : m_bus(bus), m_addr(addr) {}

bool Mpu6050::begin() {
    uint8_t id = 0;
    if (!m_bus.read(m_addr, kWhoAmI, &id, 1) || id != 0x68) {
        ESP_LOGE(kTag, "WHO_AM_I mismatch: 0x%02X", id);
        return false;
    }

    m_bus.write(m_addr, kPowerMgmt, 0x80);  // 复位
    vTaskDelay(pdMS_TO_TICKS(100));
    m_bus.write(m_addr, kPowerMgmt, 0x01);      // 时钟源 PLL
    m_bus.write(m_addr, kSampleRateDiv, 19);    // 1 kHz / (1+19) = 50 Hz
    m_bus.write(m_addr, kConfig, 0x04);         // DLPF 21 Hz
    m_bus.write(m_addr, kGyroConfig, 0x08);     // ±500 deg/s
    m_bus.write(m_addr, kAccelConfig, 0x08);    // ±4 g

    uint8_t config = 0;
    if (!m_bus.read(m_addr, kConfig, &config, 1) || config != 0x04) {
        ESP_LOGE(kTag, "config readback failed: 0x%02X", config);
        return false;
    }

    ESP_LOGI(kTag, "ready at 0x%02X", m_addr);
    return true;
}

bool Mpu6050::read(ImuFrame& frame) {
    uint8_t b[14] = {};
    if (!m_bus.read(m_addr, kAccelXoutH, b, sizeof(b))) {
        return false;
    }

    frame.ax = static_cast<float>(toInt16(b[0], b[1])) / 8192.0f;   // ±4 g
    frame.ay = static_cast<float>(toInt16(b[2], b[3])) / 8192.0f;
    frame.az = static_cast<float>(toInt16(b[4], b[5])) / 8192.0f;
    frame.gx = static_cast<float>(toInt16(b[8], b[9])) / 65.5f;     // ±500 deg/s
    frame.gy = static_cast<float>(toInt16(b[10], b[11])) / 65.5f;
    frame.gz = static_cast<float>(toInt16(b[12], b[13])) / 65.5f;
    frame.timestampUs = esp_timer_get_time();
    return true;
}

}  // namespace hb
