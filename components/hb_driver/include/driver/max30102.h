#pragma once

#include <cstdint>

#include "common/types.h"
#include "driver/sensor.h"
#include "hal/i2c_bus.h"

namespace hb {

/// MAX30102：SpO2 模式，100 Hz，18 位，FIFO 批量读取。
class Max30102 final : public Sensor {
public:
    explicit Max30102(I2CBus& bus, uint8_t addr = 0x57);

    bool begin() override;

    /// 读取 FIFO，最多 maxSamples 个。返回实际读到的样本数。
    /// 时间戳由 SensorHub 统一赋值，这里保持 0。
    int readFifo(PpgSample* out, int maxSamples);

    /// 设置 LED 电流，单位 0.2 mA（0x24 = 7.2 mA）。P4 调参用。
    bool setLedCurrent(uint8_t red, uint8_t ir);

    const char* name() const override { return "MAX30102"; }
    uint8_t address() const override { return m_addr; }

private:
    I2CBus& m_bus;
    uint8_t m_addr;
};

}  // namespace hb
