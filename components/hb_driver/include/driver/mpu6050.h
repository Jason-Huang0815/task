#pragma once

#include <cstdint>

#include "common/types.h"
#include "driver/sensor.h"
#include "hal/i2c_bus.h"

namespace hb {

/// MPU6050：±4 g、±500 deg/s、DLPF 21 Hz、50 Hz 采样。
class Mpu6050 final : public Sensor {
public:
    explicit Mpu6050(I2CBus& bus, uint8_t addr = 0x68);

    bool begin() override;
    bool read(ImuFrame& frame);

    const char* name() const override { return "MPU6050"; }
    uint8_t address() const override { return m_addr; }

private:
    I2CBus& m_bus;
    uint8_t m_addr;
};

}  // namespace hb
