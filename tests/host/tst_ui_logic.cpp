#include <cstdint>
#include <cstring>
#include <string>

#include "framework.h"
#include "ui/event_queue.h"
#include "ui/model.h"
#include "ui/nav_model.h"

// 纯逻辑用例：格式化函数、导航环绕/锁定、事件队列溢出、久坐阶段映射。
// 这些代码不碰 LVGL，所以在主机上用 MSVC 直接编译运行。

namespace ui = hb::ui;

namespace {

std::string text(const char* buffer) { return std::string(buffer); }

hb::Event makeEvent(hb::EventType type, int32_t value) {
    hb::Event event{};
    event.type = type;
    event.value = value;
    return event;
}

}  // namespace

TEST_CASE(fmt_steps_formats_unsigned) {
    char buffer[16] = {};
    CHECK(ui::fmtSteps(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("0"));
    CHECK(ui::fmtSteps(12345, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("12345"));
    CHECK(ui::fmtSteps(65535, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("65535"));
}

TEST_CASE(fmt_hr_hides_disabled_and_invalid) {
    ui::Model model;
    char buffer[16] = {};

    model.hrPageEnabled = true;
    model.hrValid = true;
    model.hr = 72;
    CHECK(ui::fmtHr(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("72"));

    model.hrValid = false;
    CHECK(ui::fmtHr(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));

    model.hrValid = true;
    model.hrPageEnabled = false;
    CHECK(ui::fmtHr(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));
}

TEST_CASE(fmt_spo2_hides_disabled_and_invalid) {
    ui::Model model;
    char buffer[16] = {};

    model.hrPageEnabled = true;
    model.spo2Valid = true;
    model.spo2 = 98;
    CHECK(ui::fmtSpo2(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("98"));

    model.spo2Valid = false;
    CHECK(ui::fmtSpo2(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));

    model.spo2Valid = true;
    model.hrPageEnabled = false;
    model.spo2 = 120;  // 超量程也要显示占位符
    CHECK(ui::fmtSpo2(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));
}

TEST_CASE(fmt_temp_truncates_to_one_decimal) {
    ui::Model model;
    char buffer[16] = {};

    model.tempPageEnabled = true;
    model.tempValid = true;
    model.tempX100 = 3650;
    CHECK(ui::fmtTemp(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("36.5"));

    model.tempX100 = 3699;  // 截断，不四舍五入
    CHECK(ui::fmtTemp(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("36.9"));

    model.tempX100 = -12;
    CHECK(ui::fmtTemp(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("-0.1"));

    model.tempValid = false;
    CHECK(ui::fmtTemp(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));

    model.tempValid = true;
    model.tempPageEnabled = false;
    CHECK(ui::fmtTemp(model, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("--"));
}

TEST_CASE(fmt_clock_uses_utc8) {
    char buffer[16] = {};

    CHECK(ui::fmtClock(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("08:00"));

    CHECK(ui::fmtClock(3600, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("09:00"));

    CHECK(ui::fmtClock(59, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("08:00"));

    // 2023-11-14T22:13:20Z -> 北京时间 2023-11-15 06:13
    CHECK(ui::fmtClock(1700000000u, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("06:13"));
}

TEST_CASE(fmt_date_and_weekday) {
    char buffer[24] = {};

    CHECK(ui::fmtDate(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("1月1日"));

    CHECK(ui::fmtWeekday(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("星期四"));

    CHECK(ui::fmtDate(1700000000u, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("11月15日"));

    CHECK(ui::fmtWeekday(1700000000u, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("星期三"));
}

TEST_CASE(fmt_sitting_distance_kcal_active) {
    char buffer[16] = {};

    CHECK(ui::fmtSitting(45, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("45"));

    CHECK(ui::fmtDistance(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("0.00"));

    CHECK(ui::fmtDistance(999, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("0.99"));

    CHECK(ui::fmtDistance(3150, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("3.15"));

    CHECK(ui::fmtKcal(123, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("123"));

    CHECK(ui::fmtActiveMin(32, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("32"));
}

TEST_CASE(fmt_delta_is_signed) {
    char buffer[16] = {};

    CHECK(ui::fmtDelta(35, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("+0.35"));

    CHECK(ui::fmtDelta(-12, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("-0.12"));

    CHECK(ui::fmtDelta(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("0.00"));

    CHECK(ui::fmtDelta(-150, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("-1.50"));
}

TEST_CASE(fmt_sync_text) {
    char buffer[16] = {};

    CHECK(ui::fmtSync(0, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("待同步"));

    CHECK(ui::fmtSync(1, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("同步中"));

    CHECK(ui::fmtSync(2, buffer, sizeof(buffer)));
    CHECK_EQ(text(buffer), std::string("已同步"));

    CHECK(ui::fmtSync(9, buffer, sizeof(buffer)));  // 未知状态按待同步处理
    CHECK_EQ(text(buffer), std::string("待同步"));
}

TEST_CASE(fmt_reports_small_buffer) {
    char tiny[4] = {};

    CHECK(!ui::fmtSteps(12345, tiny, sizeof(tiny)));
    CHECK(!ui::fmtClock(0, tiny, sizeof(tiny)));
    CHECK(!ui::fmtSteps(1, tiny, 0));
    CHECK(!ui::fmtSteps(1, nullptr, 8));
    CHECK(!ui::fmtClock(0, nullptr, 8));
}

TEST_CASE(sit_phase_text_covers_all_states) {
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::WearOff)), std::string("未佩戴"));
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::Active)), std::string("活动中"));
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::Sitting)), std::string("久坐中"));
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::Alerting)), std::string("提醒中"));
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::Snoozed)), std::string("已稍后"));
    CHECK_EQ(text(ui::sitPhaseText(ui::SitPhase::Quiet)), std::string("免打扰"));
}

TEST_CASE(sit_phase_matches_service_ordinals) {
    // 与 hb_service::SitState 的枚举顺序一一对应，映射靠这个契约。
    static const uint8_t kServiceRaw[] = {0, 1, 2, 3, 4, 5};
    static const ui::SitPhase kPhases[] = {
        ui::SitPhase::WearOff, ui::SitPhase::Active,  ui::SitPhase::Sitting,
        ui::SitPhase::Alerting, ui::SitPhase::Snoozed, ui::SitPhase::Quiet,
    };
    const int tableSize = static_cast<int>(sizeof(kPhases) / sizeof(kPhases[0]));
    int seen = 0;
    for (int i = 0; i < tableSize; ++i) {
        CHECK_EQ(static_cast<uint8_t>(kPhases[i]), kServiceRaw[i]);
        ++seen;
    }
    CHECK_EQ(seen, ui::kSitPhaseCount);

    CHECK_EQ(ui::sitPhaseFromRaw(0), ui::SitPhase::WearOff);
    CHECK_EQ(ui::sitPhaseFromRaw(2), ui::SitPhase::Sitting);
    CHECK_EQ(ui::sitPhaseFromRaw(5), ui::SitPhase::Quiet);
    CHECK_EQ(ui::sitPhaseFromRaw(6), ui::SitPhase::WearOff);  // 越界回落
}

TEST_CASE(nav_next_prev_wrap) {
    ui::NavModel nav;
    CHECK_EQ(nav.current(), ui::PageId::Watch);

    for (int i = 1; i < ui::kPageCount; ++i) {
        nav.next();
        CHECK_EQ(nav.current(), ui::NavModel::at(i));
    }
    CHECK_EQ(nav.current(), ui::PageId::Settings);

    nav.next();  // 环绕回表盘
    CHECK_EQ(nav.current(), ui::PageId::Watch);
    CHECK_EQ(nav.index(), 0);

    nav.prev();
    CHECK_EQ(nav.current(), ui::NavModel::at(ui::kPageCount - 1));
    CHECK_EQ(nav.current(), ui::PageId::Settings);
}

TEST_CASE(nav_home_and_lock) {
    ui::NavModel nav;
    nav.next();
    nav.next();
    nav.home();
    CHECK_EQ(nav.current(), ui::PageId::Watch);

    nav.setLocked(true);
    CHECK(nav.locked());
    nav.next();
    CHECK_EQ(nav.current(), ui::PageId::Watch);
    nav.prev();
    CHECK_EQ(nav.current(), ui::PageId::Watch);
    nav.home();
    CHECK_EQ(nav.current(), ui::PageId::Watch);

    nav.setLocked(false);
    nav.next();
    CHECK_EQ(nav.current(), ui::PageId::Sport);
}

TEST_CASE(nav_at_wraps_negative_and_overflow) {
    CHECK_EQ(ui::NavModel::at(-1), ui::PageId::Settings);
    CHECK_EQ(ui::NavModel::at(-7), ui::PageId::Settings);
    CHECK_EQ(ui::NavModel::at(ui::kPageCount), ui::PageId::Watch);
    CHECK_EQ(ui::NavModel::at(ui::kPageCount + 2), ui::PageId::Heart);
    CHECK_EQ(ui::NavModel::indexOf(ui::PageId::Temp), 3);
}

TEST_CASE(event_queue_is_fifo) {
    ui::EventQueue queue;
    hb::Event out{};

    CHECK_EQ(queue.size(), static_cast<size_t>(0));
    CHECK(!queue.pop(out));

    CHECK(queue.push(makeEvent(hb::EventType::Step, 1)));
    CHECK(queue.push(makeEvent(hb::EventType::MinuteTick, 2)));
    CHECK_EQ(queue.size(), static_cast<size_t>(2));

    CHECK(queue.pop(out));
    CHECK_EQ(out.type, hb::EventType::Step);
    CHECK_EQ(out.value, static_cast<int32_t>(1));

    CHECK(queue.pop(out));
    CHECK_EQ(out.type, hb::EventType::MinuteTick);
    CHECK_EQ(out.value, static_cast<int32_t>(2));

    CHECK(!queue.pop(out));
    CHECK_EQ(queue.dropped(), static_cast<uint32_t>(0));
}

TEST_CASE(event_queue_drops_oldest_when_full) {
    ui::EventQueue queue;
    hb::Event out{};

    for (int i = 0; i < ui::EventQueue::kCapacity; ++i) {
        CHECK(queue.push(makeEvent(hb::EventType::PpgSample, i)));
    }
    CHECK_EQ(queue.size(), static_cast<size_t>(ui::EventQueue::kCapacity));
    CHECK_EQ(queue.dropped(), static_cast<uint32_t>(0));

    CHECK(!queue.push(makeEvent(hb::EventType::PpgSample, 99)));  // 满时返回 false
    CHECK_EQ(queue.size(), static_cast<size_t>(ui::EventQueue::kCapacity));
    CHECK_EQ(queue.dropped(), static_cast<uint32_t>(1));

    CHECK(queue.pop(out));
    CHECK_EQ(out.value, static_cast<int32_t>(1));  // 最旧的一条被丢掉

    CHECK(queue.push(makeEvent(hb::EventType::PpgSample, 100)));
    CHECK_EQ(queue.size(), static_cast<size_t>(ui::EventQueue::kCapacity));
    CHECK_EQ(queue.dropped(), static_cast<uint32_t>(1));
}

TEST_CASE(event_queue_keeps_alert_payload) {
    ui::EventQueue queue;
    hb::Event alert{};
    alert.type = hb::EventType::Alert;
    alert.value = 47;
    alert.alert = hb::AlertKind::Fall;
    CHECK(queue.push(alert));

    hb::Event out{};
    CHECK(queue.pop(out));
    CHECK_EQ(out.type, hb::EventType::Alert);
    CHECK_EQ(out.value, static_cast<int32_t>(47));
    CHECK_EQ(out.alert, hb::AlertKind::Fall);
}
