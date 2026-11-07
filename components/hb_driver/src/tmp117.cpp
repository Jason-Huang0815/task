#include "driver/tmp117.h"

#include "esp_log.h"

namespace hb {
namespace {
const char* kTag = "tmp117";
constexpr uint8_t kDeviceId = 0x0F;
constexpr uint8_t kTempResult = 0x00;
constexpr float kLsbCelsius = 0.0078125f;
}  // namespace

Tmp117::Tmp117(I2CBus& bus, uint8_t addr) : m_bus(bus), m_addr(addr) {}

bool Tmp117::begin() {
    uint8_t id[2] = {};
    if (!m_bus.read(m_addr, kDeviceId, id, sizeof(id))) {
        return false;
    }
    const uint16_t deviceId = static_cast<uint16_t>((id[0] << 8) | id[1]);
    if (deviceId != 0x0117) {
        ESP_LOGE(kTag, "Device ID mismatch: 0x%04X", deviceId);
        return false;
    }
    ESP_LOGI(kTag, "ready at 0x%02X", m_addr);
    return true;
}

bool Tmp117::readCelsius(float& celsius) {
    uint8_t b[2] = {};
    if (!m_bus.read(m_addr, kTempResult, b, sizeof(b))) {
        return false;
    }
    const int16_t raw = static_cast<int16_t>((b[0] << 8) | b[1]);
    celsius = static_cast<float>(raw) * kLsbCelsius;
    return true;
}

}  // namespace hb
