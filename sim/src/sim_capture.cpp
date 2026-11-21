#include <cstdio>
#include <cstring>
#include <vector>

#include <array>
#include <cstdint>

namespace sim {

namespace {

uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t len) {
    static std::array<uint32_t, 256> table{};
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) != 0u ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
            }
            table[i] = c;
        }
        ready = true;
    }
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8u);
    }
    return crc;
}

uint32_t adler32(const uint8_t* data, size_t len) {
    uint32_t a = 1;
    uint32_t b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16u) | a;
}

void putU32BE(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24u) & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 16u) & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 8u) & 0xFFu));
    out.push_back(static_cast<uint8_t>(value & 0xFFu));
}

void writeChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& body) {
    putU32BE(out, static_cast<uint32_t>(body.size()));
    const size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), body.begin(), body.end());
    const uint32_t crc =
        crc32Update(0xFFFFFFFFu, out.data() + start, out.size() - start) ^ 0xFFFFFFFFu;
    putU32BE(out, crc);
}

/// zlib 容器 + 全 stored 块（不压缩）。截图只有几百 KB，够快也绝不会编坏。
std::vector<uint8_t> zlibStore(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> out;
    out.push_back(0x78);  // CMF: deflate, 32K window
    out.push_back(0x01);  // FLG: 无字典，最快压缩

    size_t offset = 0;
    while (offset < raw.size()) {
        const size_t remaining = raw.size() - offset;
        const uint16_t len = static_cast<uint16_t>(remaining > 65535u ? 65535u : remaining);
        const bool last = (offset + len) >= raw.size();
        out.push_back(last ? 0x01 : 0x00);
        out.push_back(static_cast<uint8_t>(len & 0xFFu));
        out.push_back(static_cast<uint8_t>((len >> 8u) & 0xFFu));
        out.push_back(static_cast<uint8_t>(~len & 0xFFu));
        out.push_back(static_cast<uint8_t>((~len >> 8u) & 0xFFu));
        out.insert(out.end(), raw.begin() + static_cast<long>(offset),
                   raw.begin() + static_cast<long>(offset + len));
        offset += len;
    }
    putU32BE(out, adler32(raw.data(), raw.size()));
    return out;
}

}  // namespace

/// LVGL 的 ARGB8888 在内存里是 B,G,R,A（小端 lv_color32_t），PNG 要 R,G,B,A。
bool savePng(const char* path, const uint8_t* argb, int width, int height) {
    if (path == nullptr || argb == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    std::vector<uint8_t> raw;
    raw.reserve(static_cast<size_t>(height) * (1u + 4u * static_cast<size_t>(width)));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);  // 行过滤：None
        const uint8_t* row = argb + static_cast<size_t>(y) * static_cast<size_t>(width) * 4u;
        for (int x = 0; x < width; ++x) {
            raw.push_back(row[x * 4 + 2]);  // R
            raw.push_back(row[x * 4 + 1]);  // G
            raw.push_back(row[x * 4 + 0]);  // B
            raw.push_back(0xFF);            // A：屏幕不透明
        }
    }

    std::vector<uint8_t> png = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

    std::vector<uint8_t> ihdr;
    putU32BE(ihdr, static_cast<uint32_t>(width));
    putU32BE(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(6);  // color type: RGBA
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    writeChunk(png, "IHDR", ihdr);
    writeChunk(png, "IDAT", zlibStore(raw));
    writeChunk(png, "IEND", {});

    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    const size_t written = std::fwrite(png.data(), 1, png.size(), file);
    std::fclose(file);
    return written == png.size();
}

}  // namespace sim
