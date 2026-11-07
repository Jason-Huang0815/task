#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

namespace hb {

/// 全局唯一的 I2C 总线（Meyers 单例）。
///
/// - 所有驱动共用一条总线，一次事务持锁；
/// - 每个从机地址惰性创建 device handle；
/// - 读失败自动重试 3 次并记录地址与寄存器。
class I2CBus {
public:
    static I2CBus& instance();

    I2CBus(const I2CBus&) = delete;
    I2CBus& operator=(const I2CBus&) = delete;

    /// 初始化总线。重复调用返回 ESP_OK。
    esp_err_t begin(gpio_num_t sda, gpio_num_t scl, uint32_t hz = 400000);

    bool read(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len);
    bool write(uint8_t addr, uint8_t reg, uint8_t value);
    bool writeBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len);

    /// 扫描 0x08–0x77，日志输出在线设备（自检用）。
    void scan() const;

private:
    I2CBus() = default;

    /// 调用者必须持有 m_mutex。
    i2c_master_dev_handle_t deviceFor(uint8_t addr);

    static constexpr int kMaxDevices = 8;

    struct Device {
        uint8_t addr = 0;
        i2c_master_dev_handle_t handle = nullptr;
    };

    mutable std::mutex m_mutex;
    i2c_master_bus_handle_t m_bus = nullptr;
    uint32_t m_hz = 400000;
    Device m_devices[kMaxDevices]{};
    int m_deviceCount = 0;
};

}  // namespace hb
