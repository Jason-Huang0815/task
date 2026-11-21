#pragma once

#include <cstddef>
#include <cstdint>

// 界面数据快照与全部格式化函数。
// Model 是 hb_ui 唯一的数据入口：页面看不到服务层，只读这份快照；
// fmt_* 都是纯函数，主机测试（tests/host）可以直接覆盖。

namespace hb::ui {

/// 久坐阶段。数值与 SedentaryService::SitState 一一对应，
/// 由 hb_app 的 UiBridge 转换（hb_ui 不 include service/）。
enum class SitPhase : uint8_t {
    WearOff = 0,
    Active,
    Sitting,
    Alerting,
    Snoozed,
    Quiet,
};

constexpr int kSitPhaseCount = 6;

/// 页面可见的全部状态。所有字段都有确定的默认值，开机首帧就是合法界面。
struct Model {
    uint32_t epochSec = 0;  // UTC 秒；显示时按 UTC+8 换算
    uint16_t steps = 0;
    uint16_t cadence = 0;    // 步/分
    uint8_t hr = 0;          // 次/分
    uint8_t spo2 = 0;        // %
    int16_t tempX100 = 0;    // 摄氏度 * 100
    bool worn = false;
    SitPhase sit = SitPhase::WearOff;
    uint16_t sittingMin = 0;
    uint8_t snoozeCount = 0;
    uint8_t snoozeLeftMin = 0;
    bool alerting = false;
    uint8_t batteryPct = 100;
    uint8_t brightness = 80;
    uint8_t thresholdMin = 45;
    uint8_t vibrationLevel = 2;
    bool wristRaise = true;
    uint8_t syncState = 0;  // 0 待同步 / 1 同步中 / 2 已同步
    bool btConnected = false;
    uint32_t distanceM = 0;
    uint16_t kcal = 0;
    uint16_t activeMin = 0;
    int16_t tempBaselineX100 = 3650;  // 体温基线，默认 36.50 ℃
    bool hrPageEnabled = false;
    bool tempPageEnabled = false;
    bool hrValid = false;
    bool spo2Valid = false;
    bool tempValid = false;
    uint16_t sevenDay[7] = {0, 0, 0, 0, 0, 0, 0};  // [6] 为今天
};

/// SitState 的原始值 -> SitPhase。越界值回落到 WearOff，便于测试与容错。
SitPhase sitPhaseFromRaw(uint8_t raw);

bool fmtSteps(uint16_t steps, char* out, size_t n);
bool fmtHr(const Model& model, char* out, size_t n);
bool fmtSpo2(const Model& model, char* out, size_t n);
bool fmtTemp(const Model& model, char* out, size_t n);
bool fmtClock(uint32_t epochSec, char* out, size_t n);
bool fmtDate(uint32_t epochSec, char* out, size_t n);
bool fmtWeekday(uint32_t epochSec, char* out, size_t n);
bool fmtSitting(uint16_t minutes, char* out, size_t n);
bool fmtDistance(uint32_t meters, char* out, size_t n);
bool fmtKcal(uint16_t kcal, char* out, size_t n);
bool fmtActiveMin(uint16_t minutes, char* out, size_t n);
bool fmtDelta(int16_t deltaX100, char* out, size_t n);
bool fmtSync(uint8_t state, char* out, size_t n);

const char* sitPhaseText(SitPhase phase);

}  // namespace hb::ui
