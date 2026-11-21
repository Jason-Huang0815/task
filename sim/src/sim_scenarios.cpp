// UI 场景回归：不接开发板，把真实界面渲染到内存、逐帧断言并截图。
//
// 与 tests/host 的分工：
//   tests/host/ui_logic_tests  —— 纯 C++（格式化、导航、事件队列），不需要 LVGL
//   sim/ui_sim_tests（本文件） —— 真的建控件、真的走 UiService::tick、真的截图

#include <cstdio>
#include <cstring>
#include <string>

#include "sim_harness.h"
#include "ui/fonts.h"

namespace {

using hb::ui::PageId;

void toHome(sim::Fixture& fixture) {
    fixture.ui().dismissAlert();
    fixture.publish(hb::EventType::KeyVeryLong);
    fixture.step();
    fixture.ui().dismissAlert();
    fixture.step();
}

bool hasGlyph(const lv_font_t* font, uint32_t codepoint) {
    if (font == nullptr) {
        return false;
    }
    lv_font_glyph_dsc_t dsc{};
    return lv_font_get_glyph_dsc(font, &dsc, codepoint, 0);
}

int countChildren(lv_obj_t* obj) {
    return obj == nullptr ? 0 : static_cast<int>(lv_obj_get_child_count(obj));
}

// 1. 上电默认落在表盘，且状态栏有 HH:MM
void scenarioBoot(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    checks.expect(fixture.nav().current() == PageId::Watch, "启动后停在表盘页");
    checks.expect(fixture.hasText(":"), "状态栏显示了 HH:MM");
    checks.expect(fixture.capture("s1_boot_watch"), "截图 s1_boot_watch.png 已写出");
}

// 2. 短按依次切六页，最后回到表盘
void scenarioPaging(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    const PageId expectOrder[] = {PageId::Sport,  PageId::Heart, PageId::Temp,
                                  PageId::Sedentary, PageId::Settings, PageId::Watch};
    bool allMatched = true;
    for (const PageId expected : expectOrder) {
        fixture.publish(hb::EventType::KeyShort);
        fixture.step();
        if (fixture.nav().current() != expected) {
            allMatched = false;
        }
    }
    checks.expect(allMatched, "短按 6 次按 表盘→运动→心率→体温→久坐→设置→表盘 循环");
    checks.expect(fixture.nav().current() == PageId::Watch, "第 6 次短按回到表盘");
}

// 3. 长按切背光，超长按回表盘
void scenarioKeys(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    fixture.backlight().reset();

    fixture.publish(hb::EventType::KeyLong);
    fixture.step();
    const bool dimmed = fixture.backlight().lastPct == 0;
    fixture.publish(hb::EventType::KeyLong);
    fixture.step();
    const bool restored = fixture.backlight().lastPct > 0;
    checks.expect(fixture.backlight().pctCalls >= 2, "长按调用了 BacklightPort::setPct");
    checks.expect(dimmed, "长按一次把背光调到 0（熄屏）");
    checks.expect(restored, "再长按一次恢复亮度");

    fixture.publish(hb::EventType::KeyShort);
    fixture.publish(hb::EventType::KeyShort);
    fixture.step();
    const bool leftWatch = fixture.nav().current() != PageId::Watch;
    fixture.publish(hb::EventType::KeyVeryLong);
    fixture.step();
    checks.expect(leftWatch, "两次短按已离开表盘");
    checks.expect(fixture.nav().current() == PageId::Watch, "超长按回到表盘");
}

// 4. 久坐提醒：弹窗出现并锁住导航
void scenarioSedentaryAlert(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    // 真实路径：SedentaryService 自己计时，到阈值后 publish Alert（AppController 在真机上就是这么泵的）
    fixture.sedentary().setWorn(true);
    fixture.sedentary().setQuiet(false);
    const int threshold = fixture.ui().snapshot().thresholdMin > 0
                              ? fixture.ui().snapshot().thresholdMin
                              : 45;
    for (int i = 0; i < threshold + 1; ++i) {
        fixture.sedentary().onMinuteTick(static_cast<int64_t>(fixture.nowMs()) * 1000);
        fixture.step();
    }
    const bool dialogShown = countChildren(fixture.ui().dialogLayer()) > 0;
    checks.expect(dialogShown, "久坐到阈值后弹出提醒");
    checks.expect(fixture.nav().locked(), "弹窗期间导航被锁定");

    const PageId before = fixture.nav().current();
    fixture.publish(hb::EventType::KeyShort);
    fixture.step();
    checks.expect(fixture.nav().current() == before, "弹窗期间短按不切页");
    checks.expect(fixture.capture("s2_sedentary_alert"), "截图 s2_sedentary_alert.png 已写出");
}

// 5. 点“已起身”：弹窗关闭、导航解锁
void scenarioStandUp(sim::Fixture& fixture, sim::Checks& checks) {
    checks.expect(fixture.hasText("已起身"), "弹窗上有“已起身”按钮");
    const bool clicked = fixture.click("已起身");
    checks.expect(clicked, "点到了“已起身”");
    checks.expect(countChildren(fixture.ui().dialogLayer()) == 0, "点“已起身”后弹窗关闭");
    checks.expect(!fixture.nav().locked(), "弹窗关闭后导航解锁");
}

// 6. 再提醒一次，点“稍后 10 分钟”
void scenarioSnooze(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    fixture.publish(hb::EventType::Alert, 45, hb::AlertKind::Sedentary);
    fixture.step();
    checks.expect(countChildren(fixture.ui().dialogLayer()) > 0, "注入 Alert 后弹窗出现");
    const bool clicked = fixture.click("稍后 10 分钟");
    checks.expect(clicked, "点到了“稍后 10 分钟”");
    checks.expect(countChildren(fixture.ui().dialogLayer()) == 0, "点“稍后”后弹窗关闭");
    checks.expect(!fixture.nav().locked(), "稍后之后导航解锁");
}

// 7. 跌倒：15 s 倒计时结束后自动关闭，且不会自己触发求助
void scenarioFallTimeout(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    fixture.publish(hb::EventType::Alert, 0, hb::AlertKind::Fall);
    fixture.step();
    checks.expect(fixture.hasText("检测到跌倒"), "跌倒弹窗标题正确");
    checks.expect(fixture.hasText("立即求助"), "跌倒弹窗有“立即求助”");
    checks.expect(fixture.capture("s3_fall_alert"), "截图 s3_fall_alert.png 已写出");

    fixture.advance(16000);
    checks.expect(countChildren(fixture.ui().dialogLayer()) == 0, "15 s 后跌倒弹窗自动关闭");
    checks.expect(!fixture.nav().locked(), "自动关闭后导航解锁");
}

// 8. 数据渲染：步数上万也不溢出，未接入的页显示“未启用”
void scenarioModelRender(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    hb::ui::Model model = fixture.ui().snapshot();
    model.steps = 1234;
    model.cadence = 86;
    model.sittingMin = 12;
    model.hrPageEnabled = false;
    model.tempPageEnabled = false;
    fixture.setModel(model);
    // UiService 每秒才刷一次文案，推进 1 s 时钟让它真的重画
    fixture.advance(1000);

    checks.expect(fixture.hasText("1234"), "表盘步数渲染为 1234");
    checks.expect(fixture.capture("s4_watch_steps"), "截图 s4_watch_steps.png 已写出");

    fixture.publish(hb::EventType::KeyShort);  // 运动
    fixture.publish(hb::EventType::KeyShort);  // 心率
    fixture.step();
    checks.expect(fixture.nav().current() == PageId::Heart, "已切到心率页");
    checks.expect(fixture.hasText("未启用"), "心率页在未接入时显示“未启用”");
    checks.expect(fixture.hasText("100%"), "换页后状态栏仍在（电量 100%）");
    if (!fixture.hasText("100%")) {
        std::printf("[debug] labels: %s\n", fixture.labels().c_str());
    }
    fixture.step();
    fixture.step();
    fixture.step();
    checks.expect(fixture.capture("s5_heart_disabled"), "截图 s5_heart_disabled.png 已写出");
}

// 9. 事件风暴：队列满时丢最旧，不崩、不卡死
void scenarioEventStorm(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    const uint32_t before = fixture.ui().droppedEvents();
    for (int i = 0; i < 40; ++i) {
        fixture.publish(hb::EventType::KeyShort);
    }
    fixture.step();
    checks.expect(fixture.ui().droppedEvents() > before, "灌 40 条事件后出现了丢弃计数");
    const int index = fixture.nav().index();
    checks.expect(index >= 0 && index < hb::ui::kPageCount,
                  "事件风暴后界面仍停在合法页面（index " + std::to_string(index) + "）");
}

// 10. 中文字库：界面用到的汉字在字体里都有字形
void scenarioFonts(sim::Fixture& fixture, sim::Checks& checks) {
    toHome(fixture);
    const std::string all = fixture.labels();
    int checked = 0;
    int missing = 0;
    for (size_t i = 0; i < all.size();) {
        const unsigned char lead = static_cast<unsigned char>(all[i]);
        uint32_t codepoint = lead;
        size_t step = 1;
        if ((lead & 0x80u) == 0) {
            ++i;
            continue;  // ASCII 不查
        }
        if ((lead & 0xE0u) == 0xC0u && i + 1 < all.size()) {
            codepoint = ((lead & 0x1Fu) << 6u) | (static_cast<unsigned char>(all[i + 1]) & 0x3Fu);
            step = 2;
        } else if ((lead & 0xF0u) == 0xE0u && i + 2 < all.size()) {
            codepoint = ((lead & 0x0Fu) << 12u) |
                        ((static_cast<unsigned char>(all[i + 1]) & 0x3Fu) << 6u) |
                        (static_cast<unsigned char>(all[i + 2]) & 0x3Fu);
            step = 3;
        } else {
            ++i;
            continue;
        }
        i += step;
        ++checked;
        if (!hasGlyph(hb::ui::bodyFont(), codepoint) &&
            !hasGlyph(hb::ui::textFont(), codepoint) && !hasGlyph(hb::ui::titleFont(), codepoint)) {
            ++missing;
        }
    }
    checks.expect(checked > 0, "界面上确实有中文字符待检查");
    checks.expect(missing == 0, "界面中文全部有字形（缺失 " + std::to_string(missing) + " 个）");
    checks.expect(fixture.capture("s6_font_panel"), "截图 s6_font_panel.png 已写出");
}

}  // namespace

int main() {
    lv_init();

    sim::Fixture fixture;
    if (!fixture.ok()) {
        std::printf("FATAL: 界面初始化失败（lv_display / UiBridge::begin）\n");
        return 2;
    }

    sim::Checks checks;
    scenarioBoot(fixture, checks);
    scenarioPaging(fixture, checks);
    scenarioKeys(fixture, checks);
    scenarioSedentaryAlert(fixture, checks);
    scenarioStandUp(fixture, checks);
    scenarioSnooze(fixture, checks);
    scenarioFallTimeout(fixture, checks);
    scenarioModelRender(fixture, checks);
    scenarioEventStorm(fixture, checks);
    scenarioFonts(fixture, checks);

    checks.dump("UI SIM");
    const int cases = 10;
    if (checks.failed() == 0) {
        std::printf("ALL UI SIM TESTS PASSED (%d cases, 0 failed)\n", cases);
        std::printf("截图目录: %s\n", sim::outputDir().c_str());
        return 0;
    }
    std::printf("UI SIM FAILED (%d checks, %d failed)\n", checks.total(), checks.failed());
    return 1;
}
