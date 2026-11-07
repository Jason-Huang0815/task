#include "hal/i2c_bus.h"

#include <cinttypes>
#include <cstring>

#include "esp_log.h"

namespace hb {
namespace {
const char* kTag = "i2c";
constexpr int kRetries = 3;
constexpr int kTimeoutMs = 50;
}  // namespace

I2CBus& I2CBus::instance() {
    static I2CBus bus;  // C++11 起局部静态初始化线程安全
    return bus;
}

esp_err_t I2CBus::begin(gpio_num_t sda, gpio_num_t scl, uint32_t hz) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bus != nullptr) {
        return ESP_OK;
    }

    m_hz = hz;
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = I2C_NUM_0;
    cfg.sda_io_num = sda;
    cfg.scl_io_num = scl;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;

    const esp_err_t err = i2c_new_master_bus(&cfg, &m_bus);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "bus init failed: %s", esp_err_to_name(err));
        m_bus = nullptr;
        return err;
    }
    ESP_LOGI(kTag, "bus ready: sda=%d scl=%d %" PRIu32 " Hz", static_cast<int>(sda),
             static_cast<int>(scl), hz);
    return ESP_OK;
}

i2c_master_dev_handle_t I2CBus::deviceFor(uint8_t addr) {
    for (int i = 0; i < m_deviceCount; ++i) {
        if (m_devices[i].addr == addr) {
            return m_devices[i].handle;
        }
    }
    if (m_bus == nullptr || m_deviceCount >= kMaxDevices) {
        return nullptr;
    }

    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = addr;
    cfg.scl_speed_hz = m_hz;

    i2c_master_dev_handle_t handle = nullptr;
    if (i2c_master_bus_add_device(m_bus, &cfg, &handle) != ESP_OK) {
        return nullptr;
    }
    m_devices[m_deviceCount].addr = addr;
    m_devices[m_deviceCount].handle = handle;
    ++m_deviceCount;
    return handle;
}

bool I2CBus::read(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    i2c_master_dev_handle_t dev = deviceFor(addr);
    if (dev == nullptr) {
        return false;
    }

    for (int attempt = 0; attempt < kRetries; ++attempt) {
        if (i2c_master_transmit_receive(dev, &reg, 1, buf, len, kTimeoutMs) == ESP_OK) {
            return true;
        }
    }
    ESP_LOGE(kTag, "read 0x%02X reg 0x%02X failed", addr, reg);
    return false;
}

bool I2CBus::write(uint8_t addr, uint8_t reg, uint8_t value) {
    return writeBytes(addr, reg, &value, 1);
}

bool I2CBus::writeBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0 || len + 1 > 32) {
        return false;
    }
    uint8_t buffer[32] = {};
    buffer[0] = reg;
    std::memcpy(&buffer[1], data, len);

    std::lock_guard<std::mutex> lock(m_mutex);
    i2c_master_dev_handle_t dev = deviceFor(addr);
    if (dev == nullptr) {
        return false;
    }

    for (int attempt = 0; attempt < kRetries; ++attempt) {
        if (i2c_master_transmit(dev, buffer, len + 1, kTimeoutMs) == ESP_OK) {
            return true;
        }
    }
    ESP_LOGE(kTag, "write 0x%02X reg 0x%02X failed", addr, reg);
    return false;
}

void I2CBus::scan() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bus == nullptr) {
        ESP_LOGW(kTag, "scan skipped: bus not ready");
        return;
    }
    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
        if (i2c_master_probe(m_bus, addr, kTimeoutMs) == ESP_OK) {
            ESP_LOGI(kTag, "found 0x%02X", addr);
        }
    }
}

}  // namespace hb
