#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace hb {

/// 定长环形窗口，用于计算均值与标准差。不做动态分配。
template <size_t N>
class RingWindow {
public:
    void push(float value) {
        m_buffer[m_head] = value;
        m_head = (m_head + 1) % N;
        if (m_count < N) {
            ++m_count;
        }
    }

    void clear() {
        m_head = 0;
        m_count = 0;
    }

    size_t size() const { return m_count; }
    bool full() const { return m_count == N; }

    float mean() const {
        if (m_count == 0) {
            return 0.0f;
        }
        float sum = 0.0f;
        for (size_t i = 0; i < m_count; ++i) {
            sum += m_buffer[i];
        }
        return sum / static_cast<float>(m_count);
    }

    float stddev() const {
        if (m_count < 2) {
            return 0.0f;
        }
        const float avg = mean();
        float acc = 0.0f;
        for (size_t i = 0; i < m_count; ++i) {
            const float d = m_buffer[i] - avg;
            acc += d * d;
        }
        return std::sqrt(acc / static_cast<float>(m_count));
    }

private:
    std::array<float, N> m_buffer{};
    size_t m_head = 0;
    size_t m_count = 0;
};

}  // namespace hb
