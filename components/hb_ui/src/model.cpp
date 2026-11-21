#include "ui/model.h"

#include <cstdio>

// Model 的全部格式化逻辑。纯函数、无状态、不碰 LVGL，
// 时区固定 UTC+8；温度按 0.1 ℃ 截断；距离按 0.01 km 保留两位。

namespace hb::ui {
namespace {

constexpr int64_t kSecondsPerDay = 86400;
constexpr int64_t kTzOffsetSec = 8 * 3600;

struct CivilDate {
    int year;
    int month;
    int day;
};

// Howard Hinnant 的 days -> civil 算法：1970-01-01 起的天数换算成年月日。
CivilDate civilFromDays(int64_t days) {
    const int64_t z = days + 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int64_t doe = z - era * 146097;
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t y = yoe + era * 400;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    const int64_t d = doy - (153 * mp + 2) / 5 + 1;
    const int64_t m = mp < 10 ? mp + 3 : mp - 9;
    CivilDate out{};
    out.year = static_cast<int>(m <= 2 ? y + 1 : y);
    out.month = static_cast<int>(m);
    out.day = static_cast<int>(d);
    return out;
}

int64_t localSeconds(uint32_t epochSec) {
    return static_cast<int64_t>(epochSec) + kTzOffsetSec;
}

// snprintf 返回值 >= n 表示被截断，统一按"写不下"处理。
bool fits(int written, size_t n) {
    return written >= 0 && static_cast<size_t>(written) < n;
}

bool ready(char* out, size_t n) { return out != nullptr && n > 0; }

}  // namespace

SitPhase sitPhaseFromRaw(uint8_t raw) {
    if (raw >= static_cast<uint8_t>(kSitPhaseCount)) {
        return SitPhase::WearOff;
    }
    return static_cast<SitPhase>(raw);
}

bool fmtSteps(uint16_t steps, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(steps)), n);
}

bool fmtHr(const Model& model, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    if (!model.hrPageEnabled || !model.hrValid) {
        return fits(std::snprintf(out, n, "%s", "--"), n);
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(model.hr)), n);
}

bool fmtSpo2(const Model& model, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    if (!model.hrPageEnabled || !model.spo2Valid) {
        return fits(std::snprintf(out, n, "%s", "--"), n);
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(model.spo2)), n);
}

bool fmtTemp(const Model& model, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    if (!model.tempPageEnabled || !model.tempValid) {
        return fits(std::snprintf(out, n, "%s", "--"), n);
    }
    const int32_t value = model.tempX100;
    const uint32_t magnitude = static_cast<uint32_t>(value < 0 ? -value : value);
    const char* sign = value < 0 ? "-" : "";
    return fits(std::snprintf(out, n, "%s%u.%u", sign, static_cast<unsigned>(magnitude / 100),
                              static_cast<unsigned>((magnitude % 100) / 10)),
                n);
}

bool fmtClock(uint32_t epochSec, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    const int64_t local = localSeconds(epochSec);
    const int hour = static_cast<int>((local / 3600) % 24);
    const int minute = static_cast<int>((local / 60) % 60);
    return fits(std::snprintf(out, n, "%02d:%02d", hour, minute), n);
}

bool fmtDate(uint32_t epochSec, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    const CivilDate date = civilFromDays(localSeconds(epochSec) / kSecondsPerDay);
    return fits(std::snprintf(out, n, "%d月%d日", date.month, date.day), n);
}

bool fmtWeekday(uint32_t epochSec, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    static const char* const kNames[7] = {"星期日", "星期一", "星期二", "星期三",
                                          "星期四", "星期五", "星期六"};
    const int64_t days = localSeconds(epochSec) / kSecondsPerDay;
    const int index = static_cast<int>(((days % 7) + 11) % 7);  // 1970-01-01 是星期四
    return fits(std::snprintf(out, n, "%s", kNames[index]), n);
}

bool fmtSitting(uint16_t minutes, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(minutes)), n);
}

bool fmtDistance(uint32_t meters, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    const uint32_t whole = meters / 1000;
    const uint32_t hundredths = (meters % 1000) / 10;
    return fits(std::snprintf(out, n, "%u.%02u", static_cast<unsigned>(whole),
                              static_cast<unsigned>(hundredths)),
                n);
}

bool fmtKcal(uint16_t kcal, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(kcal)), n);
}

bool fmtActiveMin(uint16_t minutes, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    return fits(std::snprintf(out, n, "%u", static_cast<unsigned>(minutes)), n);
}

bool fmtDelta(int16_t deltaX100, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    const int32_t value = deltaX100;
    const uint32_t magnitude = static_cast<uint32_t>(value < 0 ? -value : value);
    if (value == 0) {
        return fits(std::snprintf(out, n, "0.00"), n);
    }
    return fits(std::snprintf(out, n, "%c%u.%02u", value > 0 ? '+' : '-',
                              static_cast<unsigned>(magnitude / 100),
                              static_cast<unsigned>(magnitude % 100)),
                n);
}

bool fmtSync(uint8_t state, char* out, size_t n) {
    if (!ready(out, n)) {
        return false;
    }
    const char* text = "待同步";
    if (state == 1) {
        text = "同步中";
    } else if (state == 2) {
        text = "已同步";
    }
    return fits(std::snprintf(out, n, "%s", text), n);
}

const char* sitPhaseText(SitPhase phase) {
    switch (phase) {
        case SitPhase::WearOff:
            return "未佩戴";
        case SitPhase::Active:
            return "活动中";
        case SitPhase::Sitting:
            return "久坐中";
        case SitPhase::Alerting:
            return "提醒中";
        case SitPhase::Snoozed:
            return "已稍后";
        case SitPhase::Quiet:
            return "免打扰";
    }
    return "未佩戴";
}

}  // namespace hb::ui
