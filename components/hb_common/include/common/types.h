#pragma once

#include <cstddef>
#include <cstdint>

namespace hb {

/// 一帧 IMU 数据。加速度单位 g，角速度单位 deg/s。
struct ImuFrame {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
    int64_t timestampUs = 0;
};

/// 一个 PPG 样本。red / ir 为 MAX30102 的 18 位原始值。
struct PpgSample {
    uint32_t red = 0;
    uint32_t ir = 0;
    int64_t timestampUs = 0;
};

/// 分钟聚合记录，落盘格式固定 16 字节（协议见 docs/protocol.md）。
#pragma pack(push, 1)
struct MinuteRecord {
    uint32_t ts = 0;        // Unix 秒
    uint16_t steps = 0;     // 该分钟步数
    uint8_t hr = 0;         // 心率 bpm，0 表示无效
    uint8_t spo2 = 0;       // 血氧 %，0 表示无效
    int16_t tempX100 = 0;   // 体温 ×100，如 3650
    uint8_t sitMin = 0;     // 连续久坐分钟
    uint8_t flags = 0;      // bit0 佩戴，bit1 预留，bit2 跌倒
    uint32_t reserved = 0;  // 填 0
};
#pragma pack(pop)

static_assert(sizeof(MinuteRecord) == 16, "MinuteRecord must stay 16 bytes");
static_assert(offsetof(MinuteRecord, ts) == 0, "ts offset");
static_assert(offsetof(MinuteRecord, steps) == 4, "steps offset");
static_assert(offsetof(MinuteRecord, hr) == 6, "hr offset");
static_assert(offsetof(MinuteRecord, spo2) == 7, "spo2 offset");
static_assert(offsetof(MinuteRecord, tempX100) == 8, "tempX100 offset");
static_assert(offsetof(MinuteRecord, sitMin) == 10, "sitMin offset");
static_assert(offsetof(MinuteRecord, flags) == 11, "flags offset");
static_assert(offsetof(MinuteRecord, reserved) == 12, "reserved offset");

enum class SensorType : uint8_t {
    Imu = 0,
    Ppg,
    Temp,
};

}  // namespace hb
