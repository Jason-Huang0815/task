# 界面仿真：不烧板子也能看 UI、跑功能

本目录把 `components/hb_ui` 和 `components/hb_app`（界面部分）编成两个 Windows 程序：

| 程序 | 干什么 |
| --- | --- |
| `ui_sim_tests.exe` | 自动跑 10 个场景、38 条断言，截图存 PNG。给 CI 和答辩用。 |
| `ui_sim.exe` | 打开一个真窗口，键位模拟按键和告警，肉眼调界面。给自己用。 |

两者共用同一份界面代码，跟固件里跑的完全是同一套 `UiService`/`Screen`/`UiBridge`。
仿真只替换了两样东西：**显示驱动**（用内存帧缓冲代替 RGB 屏）和 **时间源**（可控时钟代替 `esp_timer`）。

---

## 1. 依赖

| 项目 | 要求 | 说明 |
| --- | --- | --- |
| Windows | 10/11 x64 | 只用 Win32 + GDI，不需要 SDL2 |
| Visual Studio | 2022（含 C++ 桌面开发） | 生成器 `Visual Studio 17 2022` |
| CMake | ≥ 3.20 | VS 自带或独立安装均可 |
| LVGL 源码 | 9.2.x | 见下方说明 |

**LVGL 源码从哪来**：仓库里不存 LVGL。跑过一次固件构建后，`managed_components/lvgl__lvgl/`
就会存在，仿真默认用它：

```powershell
# 在仓库根目录先构建一次固件，managed_components 就有了
idf.py build
```

如果暂时不想装 ESP-IDF，也可以手动放一份：

```powershell
git clone --branch v9.2.2 --depth 1 https://github.com/lvgl/lvgl.git sim/third_party/lvgl
cmake -S sim -B sim/build -DLVGL_ROOT=sim/third_party/lvgl
```

`sim/CMakeLists.txt` 找不到 LVGL 时会直接报错并打印这两条命令，不会静默生成一个空工程。

---

## 2. 三条命令

### 2.0 一键构建（推荐）

不用记命令，两条就够：

```powershell
# 配置 + 编译 + 跑 10 个场景（约 3-8 分钟，首次会编 LVGL）
powershell -ExecutionPolicy Bypass -File sim/build.ps1

# 直接打开交互窗口（按键 1-6/L/V/A/F/S，Esc 退出）
powershell -ExecutionPolicy Bypass -File sim/build.ps1 -Gui
```

在 VS Code 里按 `Ctrl+Shift+B`，选 **sim: 构建 + 跑场景测试**（任务定义在 `.vscode/tasks.json`，同列表里还有 **sim: 打开交互窗口**）。
`sim/build.ps1` 会自己找 LVGL：优先用 `sim/third_party/lvgl`，没有就从 `managed_components/lvgl__lvgl` 复制一份精简副本进去，所以**只要之前成功跑过一次 `idf.py build`，仿真就不需要再管 LVGL**。

下面 2.1 起的命令是它内部做的事，方便你排查或手动执行。

在**仓库根目录**执行：

```powershell
# ① 配置（首次或改过 lv_conf.h 后）
cmake -S sim -B sim/build -G "Visual Studio 17 2022" -A x64

# ② 构建（-m:8 很重要，LVGL 有 300 多个 C 文件）
cmake --build sim/build --config Release -j 8

# ③ 跑自动测试（离屏，不开窗口）
sim/build/bin/Release/ui_sim_tests.exe
```

通过时的输出：

```
--- 38 checks, 0 failed ---
ALL UI SIM TESTS PASSED (10 cases, 0 failed)
截图目录: sim/out
```

截图默认写到 `sim/out/`，可以用环境变量改：

```powershell
$env:SIM_OUT_DIR = "sim/out/$(Get-Date -Format yyyyMMdd-HHmm)"
```

### 打开交互窗口

```powershell
sim/build/bin/Release/ui_sim.exe
```

窗口标题会显示当前的背光和页号，例如
`角色 E · 界面仿真（800×480） 背光 80% 当前页 0`。

---

## 3. 键位

| 按键 | 模拟的事件 | 界面上应该看到 |
| --- | --- | --- |
| `1` – `6` | `KeyShort` | 依次切到表盘 / 运动 / 心率 / 体温 / 久坐 / 设置 |
| `L` | `KeyLong` | 背光在 0% 和用户设定值之间切换（只调背光，不熄 LVGL） |
| `V` | `KeyVeryLong` | 回表盘 |
| `A` | `Alert{Sedentary, 45}` | 久坐提醒弹窗，导航被锁 |
| `F` | `Alert{Fall}` | 跌倒确认弹窗，15 s 倒计时 |
| `S` | 步数 +100 | 表盘步数跳变 |
| `Esc` | 退出 | — |

弹窗期间按 `1`–`6` 不会切页，这是设计要求（弹窗未处理不许切页）。

---

## 4. 自动测试的 10 个场景

| 场景 | 断言要点 | 截图 |
| --- | --- | --- |
| `boot` | 表盘显示时间（含 `:`）、状态栏有电量 | `s1_boot_watch.png` |
| `paging` | 连按 6 次 `KeyShort` 依次经过六页并回到表盘 | — |
| `keys` | `KeyLong` 关背光再开；`KeyVeryLong` 回表盘 | — |
| `sedentaryAlert` | 久坐到阈值后弹窗出现、导航锁死、期间不切页 | `s2_sedentary_alert.png` |
| `standUp` | 点“已起身”后弹窗关闭、导航解锁 | — |
| `snooze` | 点“稍后 10 分钟”后弹窗关闭、计时重来 | — |
| `fallTimeout` | 跌倒弹窗有“我没事/立即求助”，15 s 后自动关闭并解锁 | `s3_fall_alert.png` |
| `modelRender` | 注入步数 1234 后表盘出现 `1234`；心率页显示“未启用” | `s4_watch_steps.png`、`s5_heart_disabled.png` |
| `eventStorm` | 连发 40 条事件，`droppedEvents()` 增长但页面状态仍合法 | — |
| `fonts` | 遍历界面上出现的每个汉字，在当前字体里都能找到字形 | `s6_font_panel.png` |

`fonts` 场景是真正在查字形，不是查字符串：它把标签里的 UTF-8 拆成码点，
逐个 `lv_font_get_glyph_dsc()`。缺一个就报 `missing glyph: 汉`，直接告诉你少哪个字。

---

## 5. 中文字体

### 5.1 为什么要有这一步

LVGL 只认编进固件的点阵字库。自带字库只有 ASCII，中文全是方框。
完整 CJK 字库有 1 MB 以上，而 Flash 给字库的预算是 200 KB 左右，所以只取界面真正用到的字。

### 5.2 生成

```powershell
powershell -ExecutionPolicy Bypass -File tools/gen_font_subset.ps1
```

需要 Node 和 `lv_font_conv`：

```powershell
npm install -g lv_font_conv
```

脚本读 `tools/font_symbols.txt`（界面里出现的全部汉字），生成 5 个字库到
`components/hb_ui/src/fonts/`：

| 文件 | 用途 | 字号 |
| --- | --- | --- |
| `hb_font_cn_16.c` | 状态栏、次要说明 | 16 px，bpp4 |
| `hb_font_cn_20.c` | 正文、列表 | 20 px，bpp4 |
| `hb_font_cn_26.c` | 卡片标题 | 26 px，bpp4 |
| `hb_font_num_48.c` | 步数、心率 | 48 px，bpp4 |
| `hb_font_num_112.c` | 表盘大数字 | 112 px，bpp2 |

合计约 **712 KB**（`112` 用 bpp2 是为了压体积，大字号不缺抗锯齿）。

### 5.3 字体从哪来

默认用系统字体：中文 `C:\Windows\Fonts\Deng.ttf`（等线），数字 `C:\Windows\Fonts\consola.ttf`（等宽）。
脚本参数可以换：

```powershell
powershell -File tools/gen_font_subset.ps1 -CnFont "C:\Windows\Fonts\simhei.ttf" -NumFont "C:\Windows\Fonts\consola.ttf"
```

> **`msyh.ttc` 不能用**：它是 TTC 字体集合，lv_font_conv 报
> `Unsupported OpenType signature ttcf`。要用微软雅黑请先在系统里导出单体 TTF。

### 5.4 怎么进构建

两个 CMake 都做了自动处理，字库在就编、不在就跳过：

- `sim/CMakeLists.txt`：`file(GLOB .../hb_ui/src/fonts/*.c)`，有文件时定义宏 `HB_UI_HAS_CN_FONT=1`（只对仿真目标）。
- `components/hb_ui/CMakeLists.txt`：同样 glob，有文件时把 `.c` 加进 `SRCS` 并对 `hb_ui` 定义 `HB_UI_HAS_CN_FONT=1`（`PUBLIC`，`hb_app` 也看得到）。

`components/hb_ui/include/ui/fonts.h` 里，宏为 1 时用 `hb_font_cn_*`，为 0 时回退 Montserrat——
所以**没生成字库也不会编译失败**，只是中文显示成方框。

生成完字库后，固件和仿真都要重新构建一次（CMake 配置会被 glob 结果改变）。

### 5.5 新增了中文文案怎么办

1. 把新字补进 `tools/font_symbols.txt`（一个文件里所有的汉字，去重）。
2. 重跑 `tools/gen_font_subset.ps1`。
3. 重编仿真，跑 `ui_sim_tests.exe`——`fonts` 场景会告诉你有没有漏。

`python tools/check_ui_deps.py` 也会核对“源码里出现的中文是否都在符号表里”，
提交前跑一次能少一次返工。

---

## 6. 常见故障

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `LVGL not found ...` | `managed_components` 里没有 LVGL | 先 `idf.py build`，或按第 1 节手动 clone |
| `error C1189: LV_OS_WINDOWS is required` | `lv_conf.h` 里 `LV_USE_WINDOWS 1` 但没给 `LV_USE_OS` | 本仓库已配好 `LV_USE_OS LV_OS_WINDOWS`；自己改 lv_conf 时别漏 |
| 窗口一片白、日志 `No draw buffer` | 用了 LVGL 自带 `lv_windows_create_display` | 本项目已改为自绘 Win32 窗口 + `StretchDIBits`，不走那条路 |
| 中文全是方框 | 字库没生成或宏没开 | 跑 `gen_font_subset.ps1`，确认 `components/hb_ui/src/fonts/` 有 5 个 `.c` |
| 编译特别慢（10 min+） | MSBuild 默认串行编 LVGL | `cmake --build ... -j 8` |
| `windows.h` 里一堆 `min/max` 报错 | 没定义 `NOMINMAX` | 本项目 CMake 已加；新加文件时别在 `windows.h` 前 include STL |
| 改了 `lv_conf.h` 后行为没变 | LVGL 目标没重编 | 删 `sim/build` 重新配置 |
| 截图里两张屏叠在一起 | 180 ms 淡入没走完 | `capture()` 已经先推 20 帧动画；自己写截图逻辑时记得照做 |

---

## 7. 这套仿真测不到什么

要如实说明，别把仿真当验收：

- **真实触摸**：GT9147 的坐标、镜像、校准都不在仿真里，那是角色 C 的驱动。
- **真实背光**：`BacklightPort` 在仿真里只记录调用，不驱动 GPIO/PWM。
- **帧率与内存**：PC 上跑的 FPS 和 ESP32-S3 的 PSRAM 带宽无关；M8 的帧率验收必须在真机上做。
- **RGB 时序**：黑屏、偏色、撕裂这类问题只有真屏能暴露。
- **传感器数据**：仿真里的步数、久坐是注入的，算法正确性归角色 D。

仿真能保证的是：**页面布局、中文、导航、按键语义、弹窗状态机、事件队列**这些纯逻辑与绘制层面的东西，
在烧板子之前就已经是对的。
