# ESP32-S3 手环界面：本地调试手册

> **适用分支**：`feature/ui-screens`。**代码位置**：`components/hb_ui`（界面框架）与 `components/hb_app`（页面与装配）。
> **读者**：第一次把这套 UI 跑到板子上的 E、负责接线的 A、负责屏和触摸的 C。
> **本机限制**：写这份文档的机器没有 ESP-IDF、没有 LVGL 源码、没有开发板、没有外网。文中 `idf.py build`、烧录、真机显示、FPS 数值都是**步骤与预期**，逐条标了「未在本机验证」。第 5 节的三组输出是本机真实执行结果，数字照抄。

---

## 1. 这份文档解决什么

一句话：**从拿到代码到屏幕亮出表盘、手指能切页**。

中间只有五件事：装对 ESP-IDF → 确认五项 sdkconfig → 把 `hb_ui`/`hb_app` 放对位置 → `build` + `flash` → 接上 A 的 `UiBridge`。顺序不能反：屏没亮之前调字体和动画都是白费。

| 你手上有什么 | 能做到哪一步 | 对应章节 |
| --- | --- | --- |
| 只有源码，没装 IDF | 跑三层静态/逻辑自检 | 2.4、5 |
| 装了 IDF，没有板 | 编译通过、看链接错误 | 2、3、4.1 |
| 有板，屏还没亮 | 点亮、触摸、看日志 | 4、6、7 |
| 屏已亮，要整机联调 | 按键、弹窗、设置、性能 | 6、7、8 |

排错顺序也固定：**供电与背光 → 帧缓冲与 PSRAM → 触摸 → LVGL 逻辑 → 视觉细节**。跳过前两层直接查代码，九成会查错方向。

---

## 2. 环境准备

### 2.1 安装 ESP-IDF 5.x

两条路，选一条，不要混装。

**A. Windows 安装器（推荐给第一次装的人）**

1. 下载 ESP-IDF Windows Installer（Espressif 官网），版本选 5.1 以上。`lvgl ~9.2` 要求 IDF ≥ 5.1。
2. 安装时勾选 `esp32s3` 目标支持；默认装到 `C:\Espressif`。
3. 装完开始菜单会有 **ESP-IDF 5.x CMD**，双击进入。

**B. 已经有人装过**

直接用现成环境，不要重装：

```powershell
# 二选一
C:\Espressif\idf_cmd_init.bat
. C:\Espressif\export.ps1
```

**验证**（两种方式都要过）：

```bash
idf.py --version
python --version
```

预期：

```text
ESP-IDF v5.2.x        # 5.1–5.3 都可能，和队友保持一致
Python 3.11.x
```

失败时看哪里：

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `idf.py` 不是内部或外部命令 | 没进 IDF 环境 | 用 ESP-IDF CMD，或先跑 `export.ps1` |
| `python` 指向 Microsoft Store | PATH 里是应用商店别名 | 在「应用执行别名」里关掉 python 别名，重开终端 |
| 多个 IDF 版本冲突 | 环境变量残留 | 新开终端，确认 `IDF_PATH` 只有一个值 |

> 未在本机验证：以上版本号是预期输出，本机没有安装 IDF。

### 2.2 设置目标芯片

```bash
cd E:\things\c++\esp32\task
idf.py set-target esp32s3
```

预期（约 10–30 秒）：

```text
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

同时会生成新的 `sdkconfig`，旧文件备份成 `sdkconfig.old`。**这一步会清掉之前的 menuconfig 改动**，所以顺序是：先 `set-target`，再改 sdkconfig。

失败时看哪里：

- `CMake Error: ... toolchain ... not found`：没进 IDF 环境。
- `Target "esp32s3" is not supported`：IDF 版本太老，换 5.x。
- 卡在 `Building for target`：杀毒软件锁了 `build/`，关掉实时扫描或换目录。

### 2.3 组件依赖怎么解析

`components/hb_ui/idf_component.yml` 已经写好依赖，不用手工克隆 LVGL：

```yaml
dependencies:
  idf: ">=5.1"
  lvgl/lvgl: "~9.2.0"
  espressif/esp_lvgl_port: "^2.0.0"
```

| 声明 | 锁定的版本 | 作用 |
| --- | --- | --- |
| `lvgl/lvgl ~9.2.0` | 9.2.x | 界面库本体，提供 `lv_obj_*` / `lv_screen_*` |
| `espressif/esp_lvgl_port ^2.0.0` | 2.x | 把 LVGL 接到 esp_lcd 的显示与触摸，并提供 `lvgl_port_lock/unlock` |
| `idf >=5.1` | 当前 IDF | 低于 5.1 的组件管理器不支持这组依赖 |

第一次 `idf.py build`（或 `idf.py reconfigure`）会联网访问 `components.espressif.com`，把源码下到工程根的 `managed_components/`，并生成 `dependencies.lock`。

验证：

```powershell
dir managed_components
```

预期能看到：

```text
lvgl__lvgl
espressif__esp_lvgl_port
```

失败时看哪里：

| 报错 | 原因 | 处理 |
| --- | --- | --- |
| `could not find a version that satisfies` | 版本号写错或镜像缺包 | 检查 yml；临时改成 `^9.2.0` 试 |
| `SSL certificate problem` / 超时 | 网络或代理 | 配 `IDF_COMPONENT_REGISTRY_URL` 镜像；或让队友把 `managed_components/` 整个拷来 |
| 改了 yml 不生效 | 用了旧 lock | 删 `dependencies.lock` 再 `idf.py reconfigure`（会升级版本，慎用） |

`managed_components/` 通常不进 Git；`dependencies.lock` 要提交，保证全队拉到同一版 LVGL。

### 2.4 代码放对位置

目标结构（只列相关部分）：

```text
task/
├─ components/
│  ├─ hb_common/        # 已有：Event / Settings / 类型
│  ├─ hb_service/       # 已有：Step / Sedentary / EventBus
│  ├─ hb_hal/           # 已有
│  ├─ hb_driver/        # 已有
│  ├─ hb_ui/            # 新增：include/ui、src、fonts、CMakeLists.txt、idf_component.yml
│  └─ hb_app/           # 合并：include/app、src、CMakeLists.txt
├─ main/
└─ sdkconfig
```

三个容易放错的地方：

1. `screens/` 属于 `hb_app`，不要塞进 `hb_ui`。`hb_ui` 一旦 include 了 `app/`，`check_ui_deps.py` 直接判失败。
2. `hb_app/CMakeLists.txt` 如果仓库里已有，**合并 SRCS**，不要整文件覆盖——A 的 `app_controller.cpp` 还要留着。
3. 字体 `.c` 默认生成到 `components/hb_ui/src/fonts/`（`gen_font_subset.ps1` 的 `$OutDir`）；`components/hb_ui/fonts/` 只有 README。当前 `hb_ui/CMakeLists.txt` 还没列字体源文件，开 `HB_UI_HAS_CN_FONT=1` 后必须自己加进 SRCS，否则链接找不到 `hb_font_cn_*`。

验证：

```bash
idf.py reconfigure
```

预期构建日志里出现 `hb_ui`、`hb_app` 两个组件名；没有 `Failed to resolve component 'hb_ui'`。

常见失败：`Failed to resolve component 'hb_ui'` 说明目录层级不对，必须是 `components/hb_ui/CMakeLists.txt`，不能是 `components/hb_ui/hb_ui/CMakeLists.txt`。

---

## 3. sdkconfig 必查五项

`idf.py menuconfig` 里逐项确认。改完保存，重新 `idf.py build` 才会生效。

| 配置项（sdkconfig 键） | 值 | 没开的后果 | 怎么确认 |
| --- | --- | --- | --- |
| `CONFIG_LV_COLOR_DEPTH_16=y` | 16 bit / RGB565 | 32 bit 时每像素 4 B，单缓冲 1,536,000 B，双缓冲 3 MB；PSRAM 带宽和容量都不够，表现为花屏、分配失败或个位数 FPS | `findstr COLOR_DEPTH sdkconfig` |
| `CONFIG_SPIRAM=y`<br>`CONFIG_SPIRAM_MODE_OCT=y`<br>`CONFIG_SPIRAM_SPEED_80M=y`<br>`CONFIG_SPIRAM_USE_MALLOC=y` | 八线 PSRAM，80 MHz | 800×480×2 = **768000 B**，内部 SRAM 放不下，`esp_lcd_rgb_panel` 申请帧缓冲失败，直接黑屏或重启；40 MHz 带宽余量小，动画掉帧 | `findstr SPIRAM sdkconfig`；启动日志 `spiram: Found 8MB PSRAM` |
| `CONFIG_LV_FONT_MONTSERRAT_48=y` | 48 px 数字字体 | 数字掉回默认 14 px，表盘数字明显偏小；若代码直接引用 `lv_font_montserrat_48` 则链接报 undefined | `findstr MONTSERRAT_48 sdkconfig` |
| `CONFIG_LV_USE_PERF_MONITOR=y` | 开 | 没有 FPS/CPU 角标，性能问题只能靠猜 | 运行后右上角应出现 `FPS:` |
| `CONFIG_LV_USE_LOG=y` | 开，级别建议 Warn | LVGL 内部的样式/对象/内存告警全部静默，断言时信息也更少 | `findstr LV_USE_LOG sdkconfig` |

两点补充：

- 本工程在 `HB_UI_HAS_CN_FONT=0` 时，`numberFont()` 和 `clockFont()` 都返回 `LV_FONT_DEFAULT`（默认 14 px）。所以「48 号没开」的直接观感是**数字变小**，不是编译报错；真机上先看字号，再怀疑字体文件。
- PSRAM 用 Octal 还是 Quad 由板子决定。ATK-MD0430R 这类 RGB 屏方案必须 Octal 80 MHz，否则 16 MHz PCLK × 16 bit ≈ 32 MB/s 的持续带宽顶不住。

> 未在本机验证：menuconfig 的具体菜单路径随 IDF 小版本变化，本文只给 sdkconfig 键名。以 `findstr` 结果为准。

---

## 4. 编译、烧录、看日志

### 4.1 编译

```bash
idf.py build
```

预期结尾：

```text
Project build complete.
Total image size: ... bytes
```

首次编译 3–8 分钟（要下 LVGL）。**失败时看第一条 error，不要看最后一条**。

| 第一条 error | 原因 | 处理 |
| --- | --- | --- |
| `lvgl.h: No such file or directory` | 依赖没解析 | 回到 2.3，看 `managed_components/` |
| `undefined reference to hb_font_cn_16` | 开了中文宏但字体 `.c` 没编 | 把字体源文件加进 `hb_ui/CMakeLists.txt` |
| `undefined reference to lvgl_port_lock` | 没链 `esp_lvgl_port` | 检查 `hb_ui` 的 `REQUIRES` 与 yml |
| `error: 'lv_scr_act' was not declared` | 用了 LVGL 8 的 API | 改成 `lv_screen_active()`；旧名在 9.x 已移除或弃用 |

### 4.2 烧录并看串口

先确认串口：设备管理器 → 端口，看到 `USB-SERIAL CH340` 或 `CP210x`，记下 COM 号。

```bash
idf.py -p COM5 flash monitor
```

预期依次出现：

```text
Connecting....
Chip is ESP32-S3 (QFN56) (revision v0.2)
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

然后应用日志开始刷。退出 monitor 用 `Ctrl+]`。

失败时看哪里：

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `Failed to connect to ESP32-S3` | 没进下载模式 / 线是充电线 | 按住 BOOT 再点复位；换数据线 |
| `could not open port COM5` | 串口被占用 | 关掉其他串口助手、另一个 monitor |
| 烧录成功但无日志 | 波特率/复位 | monitor 单独跑；按复位键 |

### 4.3 日志怎么读

正常启动应能看到 PSRAM 与应用入口：

```text
I (xxx) boot: ESP-IDF v5.x
I (xxx) spiram: Found 8MB PSRAM
I (xxx) app_main: ...
```

- `spiram` 不是 8MB 或直接失败 → 回第 3 节，PSRAM 没配对。
- 卡在 `cpu_start` 之前反复重启 → 供电/欠压，先量 3.3 V。

**LVGL 断言**长这样（具体文案随 LVGL 版本略有差异）：

```text
assert failed at .../lv_obj.c:123
```

处理顺序：

1. 拿 `文件:行号` 直接看 LVGL 源码那行在断言什么（空对象、非法样式、对象已删）。
2. 如果断言在 `lv_obj_*`，先怀疑两件事：对象被删后还被访问；在非 UI 任务里碰了 LVGL。
3. 拿到 `Backtrace:` 后用 `addr2line` 定位自己的代码：

```bash
addr2line -pfiaC -e build/task.elf 0x42012345 0x42067890
```

ELF 名以 `build/` 下实际文件为准（工程名不同则不是 `task.elf`）。

**Panic** 形如：

```text
Guru Meditation Error: Core  0 panic'ed (LoadProhibited). Exception was unhandled.
```

`LoadProhibited` 九成是空指针或野指针；`StoreProhibited` 是往只读/空地址写。monitor 一般会自动解码 backtrace，不能解码时再用上面的 `addr2line`。

**栈溢出**：

```text
***ERROR*** A stack overflow in task ui_task
```

`ui_task` 规划是 8 KB。页面多、层级深时提到 12 KB，改 A 的任务创建参数，不要改 LVGL 的堆。

### 4.4 打开 FPS 显示

1. `idf.py menuconfig`，开 `CONFIG_LV_USE_PERF_MONITOR`。
2. 重新编译烧录。
3. 屏幕右上角应出现 `FPS: 30` 和 `CPU: 45%` 一类角标（位置由 LVGL 的 `LV_USE_PERF_MONITOR_POS` 决定）。

判据：

- 表盘静止时 FPS 应接近理论值：`16 MHz / (889 × 526) ≈ 34.2`，实测 30±3 合理。
- CPU 持续 > 80% 说明每帧在重绘大面积内容，先查是不是每秒整页重建对象。
- FPS 显示 0：LVGL 任务没跑起来，或没有 `lv_timer_handler` 在循环。
- FPS 只有 10–15：先查 PSRAM 是不是 40 MHz、是不是双缓冲，再查页面里有没有大图/大面积透明叠加。

perf monitor 自身也吃 CPU，只用于相对比较，不要拿它当验收数字。

> 未在本机验证：FPS 数值与角标样式需在真机确认。

---

## 4.5 本机实测：ESP-IDF v5.4.4 全量编译通过

以上编译步骤已在 Windows + ESP-IDF v5.4.4 上跑通，下面的路径与输出可以直接对照。

| 项 | 本机实际值 |
| --- | --- |
| ESP-IDF | v5.4.4，`IDF_PATH=E:\esp\v5.4.4\esp-idf` |
| 工具目录 | `IDF_TOOLS_PATH=E:\Espressif\tools` |
| Python 环境 | `E:\Espressif\tools\python\v5.4.4\venv\Scripts\python.exe` |
| 激活脚本 | `C:\Espressif\tools\Microsoft.v5.4.4.PowerShell_profile.ps1` |
| 目标 | `esp32s3` |

```powershell
# 每个新开的 PowerShell 都要先激活
. 'C:\Espressif\tools\Microsoft.v5.4.4.PowerShell_profile.ps1'
cd E:\things\c++\esp32\task

idf.py set-target esp32s3     # 会拉组件、生成 sdkconfig
idf.py build
```

`set-target` 的关键输出（首次会下载组件，耗时几分钟）：

```text
-- LVGL version: 9.2.2
-- Components: ... espressif__esp_lvgl_port ... hb_app hb_common hb_driver hb_hal hb_service hb_ui ... lvgl__lvgl ...
-- Configuring done (93.4s)
-- Generating done (6.8s)
```

`build` 的关键输出：

```text
[108/108] ...
task.bin binary size 0x320e0 bytes. Smallest app partition is 0x400000 bytes. 0x3cdf20 bytes (95%) free.
Project build complete. To flash, run:
 idf.py -p PORT flash
```

### 4.5.1 两个只有真编译才会暴露的坑

**坑一：`esp_lvgl_port` 版本要比 LVGL 高一个小版本。**

`esp_lvgl_port` 2.8.0 起用了 `LV_COLOR_FORMAT_RGB565_SWAPPED`（LVGL 9.3 才有），而本工程锁的是 LVGL 9.2。组件管理器只看 `>=8,<10`，于是会给你装 2.9.0，编译直接报：

```text
managed_components/espressif__esp_lvgl_port/src/lvgl9/esp_lvgl_port_disp.c:301: error:
'LV_COLOR_FORMAT_RGB565_SWAPPED' undeclared (first use in this function)
```

所以 `components/hb_ui/idf_component.yml` 里写死上界：

```yaml
dependencies:
  idf: ">=5.1"
  lvgl/lvgl: "~9.2.0"
  espressif/esp_lvgl_port: ">=2.0.0,<2.8.0"
```

如果你以后要升到 LVGL 9.3，把这条上界去掉、同时把 `lvgl/lvgl` 改成 `~9.3.0`，两边一起升。

**坑二：RISC-V 下 `uint32_t` 是 `unsigned long`，不是 `unsigned int`。**

`-Wformat` 在 IDF 里是 error，于是：

```text
components/hb_ui/src/model.cpp:95:43: error: format '%u' expects argument of type
'unsigned int', but argument 5 has type 'long unsigned int' [-Werror=format=]
```

写法是显式转一次：`static_cast<unsigned>(magnitude / 100)`。同理，`char line[40]` 拼 `"久坐 %s 分钟"` 会触发 `-Werror=format-truncation`，缓冲区要留够（本工程改成 `char line[64]`）。这类问题在 MSVC 上不会出现，只有交叉编译才看得见。

### 4.5.2 这次编译证明了什么、没证明什么

- **证明**：7 个组件（含 `hb_ui` 的 8 个源文件、`hb_app` 的 9 个源文件）在 RISC-V 工具链下无警告编译通过，链接生成了 205 KB 的 `task.bin`，分区放得下（app 4 MB，剩余 95%）。
- **没证明**：屏幕点亮、触摸坐标、切页动画、FPS。这些必须在板子上看，见第 4 节的烧录步骤。

---

## 5. 不出硬件也能做的三层验证

三层从外到内：依赖方向 → 纯逻辑 → 语法与类型。**都不能替代真机**，但能在烧录前拦掉大部分低级错误。

| 层 | 命令 | 能证明 | 不能证明 |
| --- | --- | --- | --- |
| 1 分层与风格 | `python tools/check_ui_deps.py` | `hb_ui` 没越层、没有 Tab/超长行、中文字符都在字表里 | 代码能编译 |
| 2 纯逻辑单测 | `ui_logic_tests.exe` | 格式化、导航、事件队列的行为 | 页面布局、LVGL 调用 |
| 3 桩头编译 | `tools/ui_stub_check.ps1` | 16 个 TU 语法与类型正确、无警告 | 链接、运行、像素 |

### 5.1 第一层：分层与风格自检

```bash
cd E:\things\c++\esp32\task
python tools/check_ui_deps.py
```

真实输出（本机执行，照抄）：

```text
checked: 22 hb_ui files, 45 style files, 41 api files, 124 font symbols
OK: layers and style look clean
```

判据：退出码 0，且第二行是 `OK: layers and style look clean`。

它检查三件事：

1. `hb_ui` 的 include 里不能出现 `esp_`、`service/`、`driver/`、`hal/`、`app/`。
2. 所有源码不能有 Tab、行尾空格、超过 100 列的行。
3. 界面上出现的中文字符必须都在 `tools/font_symbols.txt` 里。

失败时输出形如 `path:line: ...`：

| 失败信息 | 原因 | 处理 |
| --- | --- | --- |
| `hb_ui 不允许依赖 service/...` | 在 `hb_ui` 里直接用了业务类型 | 把转换放到 `hb_app` 的 `UiBridge` |
| `出现 Tab` / `超过 100 列` | 编辑器设置不对 | 统一 4 空格、100 列 |
| `字符不在 font_symbols.txt` | 新加了中文文案 | 把字加进 `font_symbols.txt` 并重新生成字库 |

### 5.2 第二层：主机逻辑单测

```powershell
cmake -S tests/host -B tests/host/build -G "Visual Studio 17 2022" -A x64
cmake --build tests/host/build --config Release
tests\host\build\Release\ui_logic_tests.exe
```

真实输出（本机执行，照抄）：

```text
18 cases, 0 failed

ALL LOGIC TESTS PASSED
```

判据：退出码 0，`18 cases, 0 failed`，最后一行 `ALL LOGIC TESTS PASSED`。

覆盖范围：`fmt_*` 全部格式化函数、`sitPhaseText`、`NavModel` 环绕与锁、`EventQueue` 先进先出/满时丢最旧、Alert 载荷保留。**不覆盖**任何 LVGL 页面。

失败时看哪里：

- CMake 阶段 `HB_COMMON_INCLUDE does not contain common/events.h`：加 `-DHB_COMMON_INCLUDE=<仓库>/components/hb_common/include`，或改 `tests/host/CMakeLists.txt` 的默认值。
- 某个 case 打印 `FAILED` 与文件行号：按断言里的期望值/实际值改代码，不要改测试迁就实现。
- 找不到 `ui_logic_tests.exe`：确认 `--config Release`，路径是 `build/Release/` 而不是 `build/`。

### 5.3 第三层：LVGL 桩头编译

前提：在**仓库根**执行。

```powershell
powershell -ExecutionPolicy Bypass -File tools\ui_stub_check.ps1
```

真实输出（本机执行，照抄）：

```text
translation units: 16
errors=0 warnings=0
RESULT: OK (0 error, 0 warning)
```

退出码 `EXIT=0`。判据：16 个编译单元、0 error、0 warning，最后一行 `RESULT: OK`。

编译参数：MSVC `/std:c++17 /W4 /permissive- /utf-8`，include 路径含 `hb_ui`、`hb_app`、`hb_common`、`hb_service`、`hb_driver` 与 `tools/ui_stub`。16 个 TU = `hb_ui` 8 个源文件 + `hb_app` 8 个（`ui_bridge.cpp` + 7 个页面/弹层）。角色 A 的 `app_controller.cpp` 有 ESP-IDF 依赖，脚本**按设计排除**它。

失败时看哪里：

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `common/events.h` 里一堆 `C2059 语法错误:“}”` / `C2338 static_assert` | MSVC 没加 `/utf-8`，按 GBK 读 UTF-8 源码，句末“。”吃掉换行，下一行声明被并进注释 | 本仓库的 `tests/host/CMakeLists.txt` 已给 `hb_service`、`task_tests`、`hb_ui_logic`、`ui_logic_tests` 加 `/utf-8`；自建工程照抄 |
| `service/event_bus.h: No such file` | 缺 `hb_service`/`hb_driver` 头路径 | 本脚本已默认加入；自建工程要自己加 |
| `translation units` 数量对不上 | 少拷了源文件 | 对照 `docs/ui-api.md` 第 11 节的交付文件清单 |
| 出现 warning | 脚本把 warning 也判失败 | 按 `/W4` 提示改，不要关警告 |

桩头在 `tools/ui_stub/lvgl.h`，只声明工程实际用到的 LVGL 9 API。它**不证明**链接、运行时行为和像素输出——这三件事只能上板。

---

## 6. 接上 A：三处接线 + 一个转发

`AppController` 现在没有 UI 成员，`begin()`/`tick()` 里是 TODO。不接线，`UiBridge` 永远不会被构造，屏幕只有 LVGL 默认灰底。

先解释为什么必须转发，而不是在按键回调里直接 `lv_label_set_text`：

- `EventBus::publish()` 在**发布者线程**同步执行订阅者（`event_bus.h` 的注释写明）。按键任务、传感器任务都可能是发布者。
- LVGL 不是线程安全的，所有 `lv_*` 只能在 `ui_task` 里跑。
- `UiBridge::onEvent()` 只做一件事：把事件塞进 `UiService` 的 `EventQueue`（队列内部有 mutex，不碰 LVGL）。真正的 `handleEvent` 在 `ui_task` 的 `tick()` 里执行。

所以链路是：**发布者任务 → EventBus → UiBridge::onEvent → 队列 → ui_task tick → LVGL**。少任何一段，按键和告警都到不了界面。

### 6.1 改动一：`app_controller.h` 加成员

```diff
--- a/components/hb_app/include/app/app_controller.h
+++ b/components/hb_app/include/app/app_controller.h
@@ -1,7 +1,12 @@
 #pragma once
 
+#include <memory>
+
+#include "app/ui_bridge.h"
 #include "common/settings.h"
 #include "service/event_bus.h"
 #include "service/sedentary_service.h"
 #include "service/step_service.h"
+#include "ui/port.h"
@@
     EventBus& m_bus;
     Settings m_settings;
     StepService m_step;
     SedentaryService m_sedentary;
+    hb::ui::Port m_port;
+    hb::ui::BacklightPort m_backlight;
+    std::unique_ptr<hb::app::UiBridge> m_ui;
 };
```

为什么用 `unique_ptr`：`UiBridge` 头文件很重（带 7 个页面），放值成员会把整棵包含树拖进所有 include `app_controller.h` 的地方；指针还能让构造顺序可控。

### 6.2 改动二：`begin()` 里建桥并注入端口

```cpp
// 1) 临界区：绑定 esp_lvgl_port 的递归锁。
m_port.lock = []() { lvgl_port_lock(0); };
m_port.unlock = []() { lvgl_port_unlock(); };

// 2) 背光：函数名以 C 的实现为准；当前仓库还没有背光封装。
m_backlight.setPct = [](int pct) { board_backlight_set(pct); };

// 3) 建桥。四个引用必须是同一套实例，否则设置改了服务不认。
m_ui = std::make_unique<hb::app::UiBridge>(m_bus, m_step, m_sedentary, m_settings);
if (!m_ui->begin(lv_screen_active(), m_port, m_backlight)) {
    ESP_LOGE(kTag, "ui begin failed (no host screen?)");
    return false;
}

// 4) 把界面关心的事件转发进队列。回调在发布者线程执行，只做入队。
static constexpr hb::EventType kUiEvents[] = {
    hb::EventType::KeyShort,      hb::EventType::KeyLong,
    hb::EventType::KeyVeryLong,   hb::EventType::Alert,
    hb::EventType::UserStandUp,   hb::EventType::Snooze,
    hb::EventType::AlertTimeout,  hb::EventType::WearChanged,
};
for (hb::EventType type : kUiEvents) {
    m_bus.subscribe(type, [this](const hb::Event& event) {
        if (m_ui) {
            m_ui->onEvent(event);
        }
    });
}
```

三个前提：

- `lv_screen_active()` 必须已经有屏。显示和触摸由 C 通过 `esp_lvgl_port` 注册，**先初始化显示，再建桥**。
- LVGL 9 用 `lv_screen_active()`；`lv_scr_act()` 是 LVGL 8 的名字，合并旧文档代码时特别注意。
- `m_port`/`m_backlight` 必须比 `m_ui` 活得久。它们是 `AppController` 的成员，没问题；如果改成局部变量，`UiService` 里存的是指针，立刻悬空。

### 6.3 改动三：`tick()` 里驱动

```cpp
void AppController::tick() {
    // ... 先跑业务服务：读传感器、更新计步/久坐状态 ...
    if (m_ui) {
        m_ui->tick(lv_tick_get());  // 内部：setModel(buildModel()) + UiService::tick
    }
}
```

`ui_task` 的循环（A 负责创建）：

```cpp
for (;;) {
    if (lvgl_port_lock(200)) {
        controller.tick();       // 含 UiBridge::tick
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(16));  // 约 60 Hz 上限，16–33 ms 均可
}
```

`UiService::tick` 内部还会用 `Port` 再加一次锁。esp_lvgl_port 的锁是递归的，同任务重入不会死锁；这样即使有人在别处直接调 `tick`，也不至于把 LVGL 弄坏。

### 6.4 漏了会怎样

| 漏掉的部分 | 症状 | 排查 |
| --- | --- | --- |
| 头文件成员 | 编译不过；或用了局部 `UiBridge`，begin 后立刻析构 | 看 `m_ui` 生命周期 |
| `begin()` | 屏上只有默认灰底，没有状态栏/页点 | 在 begin 后打日志确认返回 true |
| `tick()` | 首帧之后不再刷新；时间不走、步数不动 | 在 tick 里计数，确认 16–33 ms 一次 |
| EventBus 转发 | 屏正常但按键无反应、久坐弹窗不出现 | 在 `UiBridge::onEvent` 打日志，看事件有没有到 |

---

## 7. 常见故障表

| 现象 | 真因 | 一步定位 | 归属 |
| --- | --- | --- | --- |
| **黑屏**（背光不亮 / 背光亮但无内容） | 背光未使能（XL9555 方向或电平、BL GPIO、PWM 占空比 0）；或帧缓冲申请失败（PSRAM 没开、768000 B 放不下） | 先把 `setPct` 强制 100 并量 BL 脚；monitor 搜 `spiram`、`esp_lcd`、`alloc` | C（背光/显示）；PSRAM 配置 A 复核 |
| **花屏 / 颜色错** | `LV_COLOR_DEPTH` 不是 16；RGB 顺序或 byte order 反；PCLK/porch 与屏不匹配；PSRAM 带宽不足 | 先画纯色页排除内容问题；`findstr COLOR_DEPTH sdkconfig`；临时改单缓冲看是否好转 | C（时序/格式）；E 提供纯色测试页 |
| **触摸坐标镜像或偏移**；或**触摸只响应一次** | GT9147 的 XY 交换/镜像位没配对；或读 `0x814E` 后没写 0 清标志（典型“只响应一次”） | 四角依次点，打印原始 `x,y`；对照 800×480；检查读状态后是否写 0 | C |
| **中文显示方框** | `HB_UI_HAS_CN_FONT=0`；或字库子集缺字 | 看 `fonts.h` 的宏；跑 `check_ui_deps.py` 看缺哪些字符；看链接的是不是 `hb_font_cn_*` | E（字表/宏）；C 确认字体已编入 |
| **切页后上一页还在** | 页面不是独立 screen，而是往同一 parent 叠对象；或切屏没走 `ScreenManager::show` | 看页面是否 `lv_obj_create(nullptr)`；在 `showPage` 打日志确认旧页 `onLeave` 被调 | E |
| **按钮点了没反应** | 触摸根本没进来；弹窗遮罩 `CLICKABLE` 吞点击；按钮回调没注册/回调为空 | 先在 `onTapped` 打日志；再看点击坐标是否落在按钮内（≥48 px） | E（逻辑）；触摸归 C |
| **跑一段时间内存下降** | `refresh()` 里创建对象/拼接字符串；页面被重复 `createScreen`；每帧重建样式 | 每 10 秒打一次 `heap_caps_get_free_size`；在 `refresh` 里断点看是否 `lv_obj_create` | E |
| **FPS 不达标** | 每帧全屏重绘；PSRAM 40 MHz；双缓冲配置不对；大面积透明叠加 | 开 perf monitor；切到静态页对比；临时关闭动画 | C（带宽/缓冲）；E（重绘面积） |

一个实用原则：**黑屏先分“背光”和“内容”**。手电照屏能看到暗纹 = 背光问题；全黑且串口有 PSRAM 报错 = 帧缓冲问题。两类问题的负责人不同，不要混着查。

---

## 8. 联调清单

### 8.1 向 C 要四样东西

| # | 要什么 | 为什么需要 | 怎么验收 |
| --- | --- | --- | --- |
| 1 | `display` / `indev` 的初始化位置（文件、函数、调用顺序） | 决定 `lv_screen_active()` 何时可用、`UiBridge::begin` 放哪 | begin 能拿到非空 screen，状态栏出现 |
| 2 | 帧缓冲几块、在 PSRAM 哪一段、每块多少字节 | 判断黑屏/花屏/内存；单缓冲 768000 B | 打印地址与大小；双缓冲应为 2×768000 |
| 3 | 触摸坐标是否 swap/mirror、原始分辨率 | 坐标镜像/偏移的直接依据 | 四角打点，坐标与屏幕方向一致 |
| 4 | 背光 GPIO/XL9555 位号、PWM 通道与频率、0% 是否真灭 | 亮度调节与长按息屏 | 0/50/100 三档目视 + 量电压 |

### 8.2 向 B、D、F 各要一项

| 对象 | 要什么 | 为什么 |
| --- | --- | --- |
| B | 按键 GPIO4 的消抖参数、短按/长按/超长按阈值、事件发布位置 | 决定 `KeyShort/KeyLong/KeyVeryLong` 的时序与手感 |
| D | 久坐阈值、稍后分钟数、`Alert` 的发布点与 `SitState` 映射 | 弹窗 60 s、页面进度环、状态文字都依赖它 |
| F | 设置持久化时机（何时 `nvs_commit`）、电量 ADC 来源、整机测试用例编号 | 改亮度重启是否保留、状态栏电量是否真实 |

---

## 9. 提交前自检

- [ ] `python tools/check_ui_deps.py` 输出 `OK: layers and style look clean`，退出码 0。
- [ ] `tests\host\build\Release\ui_logic_tests.exe` 输出 `18 cases, 0 failed` 与 `ALL LOGIC TESTS PASSED`。
- [ ] `tools\ui_stub_check.ps1` 输出 `translation units: 16`、`errors=0 warnings=0`、`EXIT=0`。
- [ ] 真机冷启动 5 s 内出现表盘；短按能环形切完 6 页；长按能灭屏/恢复；页点跟随当前页。
- [ ] monitor 连续跑 10 分钟无 assert/panic，`heap_caps_get_free_size` 不单调下降，FPS ≥ 27。

真机那一项做不到时，至少把前两项和“未验证”写进提交说明，不要口头说“应该没问题”。
