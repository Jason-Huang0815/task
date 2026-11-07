#pragma once

#include <cstdint>

#include "common/types.h"

namespace hb {

/// 单调时钟。主机测试用 FakeClock，固件用 esp_timer。
class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t nowUs() const = 0;
};

/// 记录存储接口。实现见 hb_service/record_store（P5）。
class IRecordStore {
public:
    virtual ~IRecordStore() = default;
    virtual bool append(const MinuteRecord& record) = 0;
    virtual uint32_t cursor() const = 0;
    virtual bool setCursor(uint32_t cursor) = 0;
};

}  // namespace hb
