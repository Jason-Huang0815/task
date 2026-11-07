#pragma once

#include <cstdint>

namespace hb {

/// 所有传感器的抽象基类。业务层只依赖接口，不依赖具体型号。
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual bool begin() = 0;
    virtual const char* name() const = 0;
    virtual uint8_t address() const = 0;
};

}  // namespace hb
