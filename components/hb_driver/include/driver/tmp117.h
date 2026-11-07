#pragma once

#include <cstdint>

#include "driver/sensor.h"
#include "hal/i2c_bus.h"

namespace hb {

/// TMP117：±0.1 degC，7.8125 m degC/LSB。
class Tmp117 final : public Sensor {
public:
    explicit Tmp117(I2CBus& bus, uint8_t addr = 0x48);

    bool begin() override;
    bool readCelsius(float& celsius);

    const char* name() const override { return "TMP117"; }
    uint8_t address() const override { return m_addr; }

private:
    I2CBus& m_bus;
    uint8_t m_addr;
};

}  // namespace hb
