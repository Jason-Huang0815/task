# ESP32-S3 手环界面：API 与集成文档

> **适用分支**：`feature/ui-screens`。**代码位置**：`components/hb_ui`（框架，`hb::ui`）与 `components/hb_app`（页面与装配，`hb::app`）。
> **读者**：接线的 A、写页面的 E、做显示/触摸的 C、做持久化与整机测试的 F。
> 本文所有签名都从代码抄出，和头文件一一对应；与现有团队文档不一致的地方集中在第 10 节。

---

## 1. 分层与依赖

```
main/app_main
 └─ hb_app
     ├─ AppController        外观：begin / tick / 事件路由
     ├─ UiBridge             业务数据 → Model，按钮动作 → service
     ├─ screens/             六个页面（Screen 子类）
     └─ AlertOverlay         两类告警弹窗（复用 ui::Dialog）
         │
         ├─ hb_ui ────────────┬─ hb_common（Event / Settings / 类型）
         │   UiService         └─ lvgl 9.2
         │   ScreenManager
         │   Model / fmt_*
         │   widgets / Dialog
         │   Port / BacklightPort
         │
         ├─ hb_service ──────── hb_common
         │   StepService / SedentaryService / EventBus
         │
         └─ hb_driver ───────── hb_hal ───── hb_common
```

三条硬规则，都有工具或代码兜底：

| 规则 | 含义 | 谁在守 |
| --- | --- | --- |
| `hb_ui` 只依赖 `hb_common` + `lvgl` | 不 include `service/`、`driver/`、`hal/`、`app/`，也不出现 `esp_*` | `tools/check_ui_deps.py` 静态检查 |
| `hb_ui` 不反向依赖 `hb_app` | 页面实现、业务装配都在 `hb_app` | 同上 |
| 所有 `lv_*` 只在 `ui_task` 调用 | 跨任务数据走 `EventBus → UiBridge::onEvent → UiService` 队列 | `UiService::tick` 用 `Port` 加锁；代码评审 |

这样分层的实际收益：`model.cpp`、`nav_model.cpp`、`event_queue.cpp` 不碰 LVGL，能在 PC 上编成单测（第 5 节的 18 个 case）；页面只读 `Model` 快照，换传感器、换算法不影响界面代码。

---

## 2. 公共头文件清单

| 文件 | 提供什么 | 谁会用 |
| --- | --- | --- |
| `ui/screen.h` | `Screen` 基类、生命周期、`createScreen()` | E 写页面；A 了解注册时机 |
| `ui/ui_service.h` | `UiConfig`、`UiCallbacks`、`UiService` 门面 | A 接线；E 注册页面/设回调 |
| `ui/screen_manager.h` | 页面注册、切换、刷新、当前页查询 | `UiService` 内部；E 排错 |
| `ui/model.h` | `Model`、`SitPhase`、全部 `fmt_*` 与 `sitPhaseText` | E 页面、`UiBridge`、主机单测 |
| `ui/widgets.h` | `makeCard/makeLabel/makeValue/makeButton/makeArc/makeBar`、`setText/setColor/setVisible/setArcPercent` | E 写页面 |
| `ui/dialog.h` | `DialogSpec`、`Dialog` | E 实现 `AlertOverlay` |
| `ui/port.h` | `Port`（lock/unlock）、`BacklightPort`（setPct/setPower） | A/C 注入 |
| `ui/fonts.h` | 字体访问器 `textFont/bodyFont/titleFont/numberFont/clockFont` | E |
| `ui/display_config.h` | 800×480 时序常量、`kBytesPerFrame`、`kFpsEstimate` | C 配 panel；A 评估内存 |
| `ui/lvgl_compat.h` | `loadScreen/activeScreen/deleteObj` 的 LVGL 9 封装 | `UiService`、页面 |
| `ui/event_queue.h` | 32 深度事件环形缓冲（线程安全） | `UiService`；A 理解事件路径 |
| `ui/nav_model.h` | `PageId`、环形导航、锁 | `UiService`、E |
| `ui/theme.h` | 颜色、字号、间距、`kSnoozeSec/kFallSec` | E |
| `app/ui_bridge.h` | `UiBridge`：装配 + 数据转换 + 回调实现 | A 接线；F 接同步/电量 |
| `app/screens/*.h` | 六个页面与 `AlertOverlay` | E；A 无需直接碰 |

`hb_ui` 的公共类型都在 `hb::ui`，`hb_app` 在 `hb::app`。页面文件里的 `namespace ui = hb::ui;` 只是省字符，不改变命名空间。

---

## 3. `Screen` 基类

```cpp
virtual PageId id() const = 0;
virtual const char* title() const = 0;
virtual void onCreate(lv_obj_t* parent) = 0;
virtual void onEnter() {}
virtual void onLeave() {}
virtual void onEvent(const hb::Event& event) {}
virtual void refresh(const Model& model) {}
```

一个页面 = 一块独立 screen 对象（`lv_obj_create(nullptr)`），创建后常驻；切页只做装载与淡入，不销毁重建。

| 虚函数 | 调用时机 | 职责 | 注意 |
| --- | --- | --- | --- |
| `id()` | 注册与导航时 | 返回固定 `PageId` | 六个槽位固定，重复 id 会让 `addScreen` 返回 false |
| `title()` | 编译期/调试 | 页面名 | 当前状态栏显示的是 `SitPhase` 文本，不是 title；别指望它自动上屏 |
| `onCreate(parent)` | `addScreen → createScreen()`，仅一次 | 建卡片、标签、按钮，指针存成员 | `parent` 就是本页 screen；不要在这里读传感器、不要 `loadScreen` |
| `onEnter()` | 目标页装载完成后 | 首帧刷新、启动本页动画 | 此时状态栏/页点/弹层已重挂到本屏，可安全访问 |
| `onLeave()` | 切走前，旧页仍活动 | 停动画、收起临时展开项 | **不要删控件**，页面还要回来 |
| `onEvent(event)` | 活动屏收到每个事件（先于全局按键逻辑） | 页面局部响应（展开列表、切换子项） | 不要阻塞；返回后 `UiService` 仍会处理 `KeyShort/KeyLong/KeyVeryLong/Alert` |
| `refresh(model)` | 活动屏每秒一次；切页后立即一次 | 只改字符串/颜色/百分比 | 不创建删除对象、不切屏、不读硬件；非活动页不会刷新 |

### 最小页面骨架

```cpp
// components/hb_app/include/app/screens/my_screen.h
#pragma once

#include "ui/screen.h"

namespace app {

class MyScreen : public hb::ui::Screen {
public:
    hb::ui::PageId id() const override { return hb::ui::PageId::Sport; }
    const char* title() const override { return "运动"; }

    void onCreate(lv_obj_t* parent) override;
    void refresh(const hb::ui::Model& model) override;

private:
    lv_obj_t* m_steps = nullptr;
};

}  // namespace app
```

```cpp
// components/hb_app/src/screens/my_screen.cpp
#include "app/screens/my_screen.h"

#include "ui/fonts.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace ui = hb::ui;

namespace app {

void MyScreen::onCreate(lv_obj_t* parent) {
    lv_obj_t* card = ui::makeCard(parent, ui::kSafePx, ui::kContentTopPx + 20, 752, 180);
    ui::makeLabel(card, "步数", ui::bodyFont(), ui::kColorTextMuted, 24, 18);
    m_steps = ui::makeValue(card, ui::numberFont(), ui::kColorSport, 24, 56);
    ui::makeLabel(card, "步", ui::bodyFont(), ui::kColorTextMuted, 260, 80);
}

void MyScreen::refresh(const ui::Model& model) {
    char buffer[32];
    if (ui::fmtSteps(model.steps, buffer, sizeof(buffer))) {
        ui::setText(m_steps, buffer);
    }
}

}  // namespace app
```

三句提醒：

- 上面借用 `PageId::Sport` 只是为了能编译。真要加第七页，先改 `nav_model.h` 的 `PageId` 与 `kPageCount`，同步 `UiService::attachChrome` 的页点数。
- `onEnter/onLeave/onEvent` 不写也能编译，基类给了空实现。
- 新增任何中文文案，先加进 `tools/font_symbols.txt` 再生成字库，否则 `check_ui_deps.py` 会拦下，真机上则是方框。

注册只需在 `UiBridge::begin()` 里加一行 `m_ui.addScreen(&m_myScreen)`，并把它加为 `UiBridge` 的成员；`start()` 之前必须注册好 `Watch`，否则首页无处可显示。

---

## 4. `UiService`

### 4.1 公共方法

| 签名 | 谁在何时调用 | 线程/约束 |
| --- | --- | --- |
| `bool begin(lv_obj_t* statusHost, const UiConfig& config)` | A，显示初始化后、`ui_task` 启动前 | 必须在 `ui_task` 里或 LVGL 已就绪时调用；`statusHost` 为空会退回 `compat::activeScreen()`，再空则返回 false |
| `bool addScreen(Screen* screen)` | `UiBridge::begin()`，逐页注册 | 触发 `Screen::createScreen()`；空指针/重复 id 返回 false；必须在 `start()` 前注册 `Watch` |
| `void start()` | `UiBridge::begin()` 最后 | 幂等；`nav.home()` 后显示表盘，开始 tick 循环 |
| `void tick(uint32_t nowMs)` | `ui_task`，每 16–33 ms | 内部 `ScopedLock` 持 `Port` 锁；`nowMs` 用 `lv_tick_get()` |
| `void setModel(const Model& model)` | `UiBridge::tick()` 每帧 | 内部互斥；可从任意任务调，但常规只在 `ui_task` |
| `Model snapshot() const` | 页面/内部 | 返回值拷贝，互斥保护 |
| `void postEvent(const hb::Event& event)` | `UiBridge::onEvent()` | 线程安全，只入队；队列满丢最旧并计数 |
| `void setCallbacks(const UiCallbacks& callbacks)` | `UiBridge::begin()`、页面 `setCallbacks` | 互斥保护；回调里不要再调 `setCallbacks` |
| `NavModel& nav()` | A/E 调试 | 直接改导航会在下一帧触发切屏；建议只在 `ui_task` 用 |
| `uint32_t droppedEvents() const` | 诊断 | 队列累计丢弃数，正常应为 0 |
| `lv_obj_t* dialogLayer() const` | `AlertOverlay::show()` | 切屏会重建，**不要缓存**这个指针 |
| `void dismissAlert()` | 页面/弹窗按钮/倒计时结束 | 解锁导航、隐藏弹层、回调 `hideAlert`；重复调用无副作用 |

### 4.2 `tick()` 里发生什么

顺序固定，排查时按这个顺序看：

1. 排空 `EventQueue`，逐个 `handleEvent`（先给活动页 `onEvent`，再处理按键/告警）。
2. 取 `Model` 快照；`m_brightness > 0` 时把 `model.brightness` 下发背光（长按关屏的 0 不会被覆盖）。
3. 弹窗倒计时：每秒回调一次；归零时若为久坐则先调 `snooze`，再 `dismissAlert`。
4. 导航变化则 `showPage(animate=true)`，重建状态栏/页点/弹层。
5. 每 1000 ms 或 `dirty` 时 `refresh(model)` + `updateChrome(model)`。

### 4.3 标准 `ui_task` 循环

```cpp
void ui_task(void* arg) {
    AppController& controller = *static_cast<AppController*>(arg);
    for (;;) {
        if (lvgl_port_lock(200)) {
            controller.tick();          // 内部会调 UiBridge::tick → UiService::tick
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
```

`UiService::tick` 内部还会通过 `Port` 再加一次锁。esp_lvgl_port 的锁是递归的，同任务重入没问题；如果 `Port` 没注入，`tick` 就不再额外加锁，此时必须由调用方自己保证在 `ui_task` 里。

---

## 5. `UiBridge`

```cpp
UiBridge(hb::EventBus& bus, hb::StepService& steps,
         hb::SedentaryService& sedentary, hb::Settings& settings);

bool begin(lv_obj_t* statusHost, const ui::Port& port,
           const ui::BacklightPort& backlight);

void tick(uint32_t nowMs);
void onEvent(const hb::Event& event);
void setSyncState(uint8_t state);
ui::UiService& ui();
```

| 成员 | 说明 |
| --- | --- |
| 构造参数 `settings` | **必须**与 `StepService`/`SedentaryService` 用同一个实例，否则设置页改阈值，服务层还按旧值判定 |
| `begin` | 保存端口 → `UiService::begin` → 注册 6 页 → 设回调 → `setModel(buildModel())` → `start()` → 创建 `AlertOverlay`（必须在 `start` 之后，弹层要挂到当前屏） |
| `tick` | 先更新 `activeMin`（`cadence >= 30` 才累计），再 `setModel(buildModel())`，最后 `UiService::tick`。间隔 > 5 s 视为休眠，本帧不计入活跃分钟 |
| `onEvent` | **只做 `postEvent`**。因此可以在任意任务（EventBus 发布者线程）调用，不碰 LVGL |
| `setSyncState` | 1 = 同步中，2 = 已同步；F 的同步任务回填。会进入下一帧 `Model.syncState` |
| `ui()` | 逃生口，拿到底层 `UiService`；只在确实需要 `nav()`/`dialogLayer()` 时用 |

`buildModel()` 的当前取值：步数/步频来自 `StepService`，久坐来自 `SedentaryService`，亮度/阈值/震动/抬腕来自 `Settings`，距离 = 步数 × 0.7 m，热量 = 步数 / 30，电量固定 100，心率/血氧/体温直接置 0 且 `*PageEnabled=false`（不造假数据，页面显示 `--`）。

### A 的接线步骤

1. `app_controller.h`：加 `hb::ui::Port`、`hb::ui::BacklightPort`、`std::unique_ptr<hb::app::UiBridge>` 三个成员。
2. `begin()`：设 `m_port.lock/unlock`（绑 `lvgl_port_lock(0)/unlock`）与 `m_backlight.setPct`。
3. `begin()`：`make_unique<UiBridge>(m_bus, m_step, m_sedentary, m_settings)`，再 `begin(lv_screen_active(), m_port, m_backlight)`。
4. `begin()`：对 8 类 UI 事件 `subscribe`，回调里只调 `m_ui->onEvent(event)`。
5. `tick()`：业务服务跑完后 `m_ui->tick(lv_tick_get())`。

完整 diff 见 `docs/ui-debug.md` 第 6 节，不要两处各维护一份。

---

## 6. `UiCallbacks`

11 个 `std::function`，是界面唯一能“出去”的通道。页面不直接调 service。

| 回调 | 触发场景 | 由谁实现 | 超时/未实现时 |
| --- | --- | --- | --- |
| `standUp()` | 久坐页「我起来了」；久坐弹窗主按钮「已起身」；跌倒弹窗主按钮「我没事」（复用） | `UiBridge`：`sedentary.onUserStandUp(nowUs())` + `publish(UserStandUp)` + `dismissAlert` | 空实现时弹窗不关，久坐 60 s 到点会自动 `snooze` 并关闭 |
| `snooze()` | 久坐页「稍后 10 分钟」；久坐弹窗次按钮 | `UiBridge`：`sedentary.onSnooze(nowUs())` + `publish(Snooze)` + `dismissAlert` | 空实现时 60 s 到点只关闭弹窗，状态机不进入 Snoozed |
| `syncNow()` | 设置页「同步」 | `UiBridge` 当前只置 `m_syncState = 2`；真正同步由 F 替换 | 空实现时按钮无反馈，状态仍是「待同步」 |
| `setBrightness(int pct)` | 设置页亮度 ±10，范围 1–100 | `UiBridge`：clamp 后写 `Settings.brightness` | 空实现时数值变了、背光不变 |
| `setThreshold(int min)` | 设置页久坐阈值 ±5，范围 10–120 | `UiBridge`：写 `Settings.thresholdMin` | 空实现时显示变、久坐判定仍用旧阈值 |
| `setVibration(int level)` | 设置页震动强度 ±1，范围 0–3 | `UiBridge`：写 `Settings.vibrationLevel` | 空实现时数值变、马达行为不变（B 读取） |
| `setWristRaise(bool on)` | 设置页抬腕亮屏切换 | `UiBridge`：写 `Settings.wristRaise` | 空实现时开关变、抬腕不生效 |
| `showAlert(hb::AlertKind kind, int value)` | `UiService::openAlert` | `UiBridge`：`AlertOverlay::show(kind, value)` | 空实现时没有弹窗，但导航仍被锁 60/15 s，表现像“按键失灵” |
| `updateAlertCountdown(int seconds)` | 弹窗打开立即一次，之后每秒 | `UiBridge`：`overlay.updateCountdown` | 空实现时倒计时文字停住，但到时仍会自动关闭 |
| `hideAlert()` | `dismissAlert` | `UiBridge`：`overlay.hide` | 空实现时弹窗残留、遮罩继续吞点击 |
| `fallSos()` | 跌倒弹窗「立即求助」 | `UiBridge`：`publish(KeyVeryLong, 0, AlertKind::Fall)` + `dismissAlert`；**A 必须订阅这个组合并发求助** | 空实现时只关弹窗，没有任何求助动作 |

两类弹窗的时限（`theme.h`：`kSnoozeSec = 60`、`kFallSec = 15`）：

- **久坐 60 s**：倒计时归零 → 自动调 `snooze()` → `dismissAlert()`，即“自动稍后”。
- **跌倒 15 s**：倒计时归零只关闭弹窗，**不会自动触发 `fallSos()`**。求助必须由用户按「立即求助」。如果产品要求超时自动报警，这是待补的产品逻辑，不要说成已实现。

弹窗打开期间 `nav.setLocked(true)`，短按切页无效；`KeyLong` 仍会开关背光；`KeyVeryLong` 不会回表盘（`NavModel::home()` 在锁定状态下也是空操作）。

---

## 7. `Port` / `BacklightPort`

```cpp
struct Port {
    std::function<void()> lock;
    std::function<void()> unlock;
    bool valid() const;   // lock && unlock
};

struct BacklightPort {
    std::function<void(int)> setPct;     // 0–100
    std::function<void(bool)> setPower;  // 可选
    bool valid() const;                  // 只看 setPct
};
```

A 的实现（函数名以 C 的背光封装为准，仓库当前还没有）：

```cpp
m_port.lock = []() { lvgl_port_lock(0); };
m_port.unlock = []() { lvgl_port_unlock(); };

m_backlight.setPct = [](int pct) { board_backlight_set(pct); };
m_backlight.setPower = [](bool on) { board_backlight_power(on); };  // 可选
```

不实现的后果：

| 情况 | 实际行为 | 风险 |
| --- | --- | --- |
| `Port` 两个都为空 | `ScopedLock` 跳过加锁，`tick` 直接碰 LVGL | 若渲染在另一任务，画面撕裂/崩溃；单任务轮询时暂时看不出 |
| 只填 `lock`、不填 `unlock` | 锁后永不释放 | 下一次 `lock` 死锁；`valid()` 为 false 但 `ScopedLock` 不看它，照样会锁 |
| `BacklightPort.setPct` 为空 | 亮度值照常记录，`applyBrightness` 发现指针为空就跳过 | 改亮度、长按息屏都没有硬件效果，绘制不受影响 |
| `setPower` 为空 | 无人调用它 | 无影响；`UiService` 只用 `setPct`，`setPower` 留给硬件关断 |

`UiConfig` 里存的是**指针**：`config.port = &port`。端口对象必须比 `UiService` 活得久；`UiBridge` 的 `m_port/m_backlight` 是成员，满足这个条件。

---

## 8. `Model`

`Model` 是页面能看到的全部状态，默认值保证开机首帧就是合法界面。

| 字段 | 单位/取值 | 谁填 | 无效值语义 |
| --- | --- | --- | --- |
| `epochSec` | UTC 秒 | `UiBridge`：`std::time(nullptr)` | 0 → 显示 `08:00`（UTC+8），说明 SNTP 未同步，不是 UI bug |
| `steps` | 步 | `StepService` | 超过 65535 饱和 |
| `cadence` | 步/分 | `StepService` | clamp 0–300 |
| `hr` | 次/分 | B（传感器） | `hrPageEnabled && hrValid` 才显示；否则 `--` |
| `spo2` | % | B | `hrPageEnabled && spo2Valid`；否则 `--` |
| `tempX100` | 0.01 ℃ | B | `tempPageEnabled && tempValid`；否则 `--`；36.9 ℃ = 3690 |
| `worn` | bool | `UiBridge`：`SitState != WearOff` | false 时状态栏显示「未佩戴」 |
| `sit` | `SitPhase` | `UiBridge`：`sitPhaseOf()` | 越界原始值经 `sitPhaseFromRaw` 回落 `WearOff` |
| `sittingMin` | 分 | `SedentaryService` | clamp 0–1440 |
| `snoozeCount` | 次 | `SedentaryService` | clamp 0–255 |
| `snoozeLeftMin` | 分 | `UiBridge`，仅 `Snoozed` 时 = `Settings.snoozeMin` | 其他阶段为 0；页面进度环按**硬编码 10 分钟**算比例，改 `snoozeMin` 要同步改页面 |
| `alerting` | bool | `SedentaryService::isAlerting()` | 仅状态栏/调试用 |
| `batteryPct` | % | 当前固定 100，F 接 ADC | 0–100 |
| `brightness` | % | `Settings.brightness` | 正常 1–100；长按关屏是 `UiService` 内部的 0，不写回 Model；`tick` 只在内部值 > 0 时下发给背光 |
| `thresholdMin` | 分 | `Settings.thresholdMin` | 10–120 |
| `vibrationLevel` | 档 | `Settings.vibrationLevel` | 0–3 |
| `wristRaise` | bool | `Settings.wristRaise` | — |
| `syncState` | 0/1/2 | `UiBridge::m_syncState` | 其他值按「待同步」；2 才会显示「已同步」 |
| `btConnected` | bool | 预留，当前恒 false | 状态栏暂未显示 |
| `distanceM` | 米 | `steps * 7 / 10` | 显示时转 km，两位小数 |
| `kcal` | 千卡 | `steps / 30` | — |
| `activeMin` | 分 | `UiBridge` 按步频累计 | 间隔 > 5 s 的帧不计 |
| `tempBaselineX100` | 0.01 ℃ | 默认 3650（36.50 ℃） | 用于体温偏差显示 |
| `hrPageEnabled` | bool | `UiBridge` 当前 false | **未启用约定**：页面仍存在（六页固定），数值显示 `--`，不要用 0 冒充数据 |
| `tempPageEnabled` | bool | `UiBridge` 当前 false | 同上 |
| `hrValid`/`spo2Valid`/`tempValid` | bool | B | 与 `*PageEnabled` 同时为真才显示数值 |
| `sevenDay[7]` | 步 | `UiBridge` 当前只填 `[6]=steps` | `[6]` 为今天，历史由 F 从 RecordStore 回填 |

「未启用」和「无效」是两件事：`*PageEnabled=false` 表示这条数据链还没接（硬件/算法未就绪）；`*Valid=false` 表示这次采样无效。两者都输出 `--`，但排查方向不同。

---

## 9. 事件契约

`UiService` 从队列里消费这些事件。前提是 A 已经把对应 `EventType` 转发进 `UiBridge::onEvent`。

| EventType | 谁在什么条件下发出 | `UiService` 的行为 | 备注 |
| --- | --- | --- | --- |
| `KeyShort` | 按键任务（B）：短按释放 | `nav.next()`，环形切页 | 弹窗期间 `nav` 被锁，无效 |
| `KeyLong` | 按键任务（B）：长按达到阈值 | 背光 0 ↔ 恢复（恢复值取 `Model.brightness`，为 0 则 80） | 弹窗期间仍生效；阈值由 B 定，建议 800 ms |
| `KeyVeryLong` | 按键任务（B）：超长按 | `nav.home()` 回表盘 | `fallSos` 也发这个类型，但 `alert=Fall`；A 订阅时按 `alert` 字段区分 |
| `Alert` | `SedentaryService` 坐满阈值：`publish({Alert, sittingMin, Sedentary})`；跌倒检测（未实现）应发 `{Alert, 0, Fall}` | `openAlert`：锁导航、显示弹窗、启动 60/15 s 倒计时 | 已开弹窗时忽略新 `Alert`；`value` 进入 `showAlert` 第二参 |
| `UserStandUp` | `UiBridge` 在「我起来了/已起身」时，调用服务后发出 | `dismissAlert()` | 通常弹窗已经由按钮关闭，这里是兜底 |
| `Snooze` | `UiBridge` 在「稍后 10 分钟」时发出 | `dismissAlert()` | 同上 |
| `AlertTimeout` | 设计上由业务层超时后发出；**当前仓库没有发布者** | `dismissAlert()` | 不要误以为它会自动触发；久坐超时是 `UiService` 自己调 `snooze` |
| `WearChanged` | 佩戴检测（B） | **忽略**（`default` 分支） | 佩戴状态经 `SedentaryService` 的 `SitState` 反映到 `Model.worn`，不直接改界面 |

两条工程约束：

- UI 只收活动屏的 `onEvent`；非活动页收不到，切回时靠 `refresh` 补状态。
- 队列容量 32，满时丢最旧一条并累加 `droppedEvents()`。正常交互不应丢；若持续增长，说明 `ui_task` 被阻塞或事件源频率异常。

---

## 10. 与现有文档的三处口径差异

| 文档位置 | 现文 | 建议改成 |
| --- | --- | --- |
| `docs/architecture.md` 第 1 节分层图 | `app["hb_app<br/>AppController · ScreenManager"]` | `ScreenManager` 在 `hb_ui`。改为 `app["hb_app<br/>AppController · UiBridge · screens"]`，并在 `ui["hb_ui<br/>…"]` 里加 `ScreenManager` |
| `components/hb_app/screens/README.md` 第 31 行 | 「页面切换用 `lv_scr_load_anim`，180 ms」；生命周期写作 `onCreate(void* root)`，`onEnter/onLeave` 纯虚 | LVGL 9 用 `lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_ON, 180, 0, false)`，工程内统一走 `ui::compat::loadScreen(screen, animate)`；签名改为 `onCreate(lv_obj_t* parent)`，`onEnter/onLeave` 提供默认空实现 |
| `docs/architecture.md` ADR-004（约 175–179 行） | 「`DisplayDriver` / `TouchDriver` 抽象接口已定义，后端 TODO(P1)」 | 仓库里没有这两个头文件，也没有实现。改为：「显示/触摸不做自定义抽象层，直接由 `esp_lvgl_port` + `esp_lcd` 接入；UI 只依赖 `Port`/`BacklightPort` 与 `lv_display`/`lv_indev`」 |

另外两处顺手同步，不计入上表：

- `README.md` 顶部写 LVGL 8.3，实际是 9.2 + esp_lvgl_port v2，API 不可混用。
- `architecture.md` 组件职责表里 `hb_app` 的「页面栈」应改为「页面实现与装配」；页面栈（`ScreenManager`）属于 `hb_ui`。

---

## 11. 交付文件清单

| 文件/目录 | 作用 | 归属 |
| --- | --- | --- |
| `components/hb_ui/include/ui/*.h`（14 个） | 框架公共 API：screen/ui_service/screen_manager/model/widgets/dialog/port/fonts/display_config/lvgl_compat/event_queue/nav_model/theme | E |
| `components/hb_ui/src/*.cpp`（8 个） | 上述实现，含 LVGL 兼容层与门面 | E |
| `components/hb_ui/src/fonts/`（生成物）、`components/hb_ui/fonts/README.md` | 中文子集与数字字体；`gen_font_subset.ps1` 的 `$OutDir` 默认是 `src/fonts`，CMake 里需手动加入 SRCS | E |
| `components/hb_ui/CMakeLists.txt`、`idf_component.yml` | 构建与依赖（lvgl ~9.2、esp_lvgl_port ^2.0） | E |
| `components/hb_app/include/app/ui_bridge.h`、`src/ui_bridge.cpp` | 装配、Model 转换、回调实现 | E |
| `components/hb_app/include/app/screens/*.h`、`src/screens/*.cpp` | 六页 + `AlertOverlay` | E |
| `components/hb_app/CMakeLists.txt` | 合并 SRCS（9 个源文件） | E 提供，A 合并 |
| `integration/README.md` | 给 A 的接线步骤与 sdkconfig 清单 | E 交付，A 执行 |
| `tools/check_ui_deps.py`、`font_symbols.txt`、`gen_font_subset.ps1` | 分层自检与字库生成 | E |
| `tools/ui_stub_check.ps1`、`tools/ui_stub/` | 无 LVGL 源码时的语法/类型自检 | E |
| `tests/host/` | 18 个纯逻辑用例 | E |
| `docs/ui-debug.md`、`docs/ui-api.md` | 调试手册与本 API 文档 | E |
| `components/hb_app/include/app/app_controller.h`、`src/app_controller.cpp` | 加 UI 成员、建桥、转发事件、驱动 tick | **A**（E 只给 diff） |
| 显示/触摸初始化、背光封装 | `esp_lcd` panel、GT9147、PWM | **C** |
| 传感器数据、按键事件 | 心率/血氧/体温、`KeyShort/KeyLong/KeyVeryLong` | **B** |
| 久坐阈值与 `SitState` | `SedentaryService` 与 `Alert` 发布 | **D** |
| 设置持久化、电量、整机测试 | NVS、ADC、验收记录 | **F** |
