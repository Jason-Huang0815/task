#include "driver/max30102.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hb {
namespace {
const char* kTag = "max30102";
constexpr uint8_t kPartId = 0xFF;
constexpr uint8_t kModeConfig = 0x09;
constexpr uint8_t kSpO2Config = 0x0A;
constexpr uint8_t kLed1Pa = 0x0C;
constexpr uint8_t kLed2Pa = 0x0D;
constexpr uint8_t kFifoWrite = 0x04;
constexpr uint8_t kFifoRead = 0x06;
constexpr uint8_t kFifoData = 0x07;
constexpr uint8_t kFifoConfig = 0x08;
constexpr uint8_t kIntEnable1 = 0x02;
}  // namespace

Max30102::Max30102(I2CBus& bus, uint8_t addr) : m_bus(bus), m_addr(addr) {}

bool Max30102::begin() {
    uint8_t part = 0;
    if (!m_bus.read(m_addr, kPartId, &part, 1) || part != 0x15) {
        ESP_LOGE(kTag, "Part ID mismatch: 0x%02X", part);
        return false;
    }

    m_bus.write(m_addr, kModeConfig, 0x40);  // reset
    vTaskDelay(pdMS_TO_TICKS(10));
    m_bus.write(m_addr, kFifoConfig, 0x4F);  // 平均 4、rollover、almost full
    m_bus.write(m_addr, kSpO2Config, 0x27);  // 100 Hz、411 us、18 bit
    m_bus.write(m_addr, kLed1Pa, 0x24);      // RED 7.2 mA
    m_bus.write(m_addr, kLed2Pa, 0x24);      // IR  7.2 mA
    m_bus.write(m_addr, kIntEnable1, 0xC0);  // A_FULL + PPG_RDY
    m_bus.write(m_addr, kModeConfig, 0x03);  // SpO2 模式

    ESP_LOGI(kTag, "ready at 0x%02X", m_addr);
    return true;
}

bool Max30102::setLedCurrent(uint8_t red, uint8_t ir) {
    return m_bus.write(m_addr, kLed1Pa, red) && m_bus.write(m_addr, kLed2Pa, ir);
}

int Max30102::readFifo(PpgSample* out, int maxSamples) {
    if (out == nullptr || maxSamples <= 0) {
        return 0;
    }

    uint8_t wr = 0;
    uint8_t rd = 0;
    if (!m_bus.read(m_addr, kFifoWrite, &wr, 1) || !m_bus.read(m_addr, kFifoRead, &rd, 1)) {
        return 0;
    }

    int available = (wr - rd) & 0x1F;
    if (available > maxSamples) {
        available = maxSamples;  // FIFO 溢出时丢弃最新之外的样本
    }

    int count = 0;
    for (int i = 0; i < available; ++i) {
        uint8_t b[6] = {};
        if (!m_bus.read(m_addr, kFifoData, b, sizeof(b))) {
            break;
        }
        out[i].red = ((static_cast<uint32_t>(b[0]) << 16) |
                      (static_cast<uint32_t>(b[1]) << 8) | b[2]) & 0x3FFFF;
        out[i].ir = ((static_cast<uint32_t>(b[3]) << 16) |
                     (static_cast<uint32_t>(b[4]) << 8) | b[5]) & 0x3FFFF;
        out[i].timestampUs = 0;
        ++count;
    }
    return count;
}

}  // namespace hb
