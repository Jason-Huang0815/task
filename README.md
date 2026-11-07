# task · 基于 ESP32-S3 的智能健康手环

> 专业综合实训项目骨架：**C++17 + CMake + ESP-IDF 5.x + FreeRTOS + LVGL 8.3**，PC 端 Qt 5 为选做。
> 核心闭环：**运动监测 → 久坐判定 → 震动/弹窗提醒 → 本地记录**。

<!-- 把 <owner> 换成你的 GitHub 用户名后启用
![host-tests](https://github.com/<owner>/task/actions/workflows/host-tests.yml/badge.svg)
-->

## 目录

- [0. 当前状态（先读这一段）](#0-当前状态先读这一段)
- [1. 项目目标与范围](#1-项目目标与范围)
- [2. 硬件与接线（速查）](#2-硬件与接线速查)
- [3. 软件架构](#3-软件架构)
  - [3.1 分层](#31-分层)
  - [3.2 目录](#32-目录)
- [4. 环境准备](#4-环境准备)
  - [4.1 必装](#41-必装)
  - [4.2 ESP-IDF](#42-esp-idf)
- [5. 快速开始](#5-快速开始)
  - [5.1 只跑主机测试（不需要开发板）](#51-只跑主机测试不需要开发板)
  - [5.2 固件构建与烧录](#52-固件构建与烧录)
- [6. IDE 配置](#6-ide-配置)
  - [6.1 VS Code](#61-vs-code)
  - [6.2 CLion](#62-clion)
- [7. Git 零基础（GitHub）](#7-git-零基础github)
  - [7.1 安装与身份](#71-安装与身份)
  - [7.2 起点 A：仓库已经在 GitHub 上（推荐）](#72-起点-a仓库已经在-github-上推荐)
  - [7.3 起点 B：本地已有代码，要推到 GitHub](#73-起点-b本地已有代码要推到-github)
  - [7.4 每天固定动作](#74-每天固定动作)
  - [7.5 分支与 PR 规则](#75-分支与-pr-规则)
  - [7.6 冲突怎么处理](#76-冲突怎么处理)
  - [7.7 撤销操作](#77-撤销操作)
  - [7.8 常见报错](#78-常见报错)
  - [7.9 三条红线](#79-三条红线)
- [8. 团队分工](#8-团队分工)
- [9. 工作流程](#9-工作流程)
  - [9.1 两个循环](#91-两个循环)
  - [9.2 提交信息](#92-提交信息)
  - [9.3 PR 检查清单](#93-pr-检查清单)
  - [9.4 阶段计划](#94-阶段计划)
  - [9.5 完成定义（DoD）](#95-完成定义dod)
- [10. 测试](#10-测试)
  - [10.1 主机侧](#101-主机侧)
  - [10.2 硬件侧](#102-硬件侧)
- [11. 排障](#11-排障)
- [12. 路线图](#12-路线图)
- [13. 许可与声明](#13-许可与声明)

---

## 0. 当前状态（先读这一段）

这是一个**可编译、可测试的骨架**，不是完成品。目录、接口、分层约束已经定好，算法与驱动按阶段填充。

| 模块 | 状态 | 说明 |
| --- | --- | --- |
| 主机侧构建（CMake + CTest） | ✅ 可用 | 不需要开发板，秒级回归 |
| `hb_common` 数据类型 / 事件 / 设置 / 接口 | ✅ 已实现 | 无任何 IDF、LVGL 依赖 |
| `EventBus` / `StepService` / `SedentaryService` | ✅ 已实现 | 带单元测试，可直接调参 |
| `I2CBus` / MPU6050 / MAX30102 / TMP117 | ✅ 骨架代码 | 寄存器值需按实物手册复核 |
| 显示 / 触摸 / LVGL 六页面 | ⏳ TODO(P1/P4) | 见 `components/hb_ui/`、`hb_app/screens/` |
| `SensorHub` / `RecordStore` / 同步 / 看门狗 | ⏳ TODO(P2/P5) | 见 `docs/architecture.md` 路线图 |
| Qt PC 端 | ⏳ 选做(P6) | 不在本仓库内 |

代码里的 `TODO(Px)` 都对应 `docs/architecture.md` 的阶段表。**先让主机测试跑绿，再上板。**

---

## 1. 项目目标与范围

| 类别 | 内容 | 要求 |
| --- | --- | --- |
| 必做 | 三路传感器识别、屏幕点屏与触摸、按键与马达、IMU 采样 | P1–P2 |
| 必做 | 计步（动态阈值 + 峰谷对）、久坐状态机、震动 + 全屏提醒 | P3 |
| 必做 | 表盘、运动、久坐、设置四个页面 | P4 |
| 选做 | 心率血氧（PPG）、体温、跌倒、六页全量 | P4 |
| 选做 | NVS 设置、LittleFS 分钟记录、断点续传、看门狗 | P5 |
| 选做 | Qt + SQLite 上位机、CSV 导出 | P6 |

四条底线（任何档位都必须过）：**提醒能触发、数据能显示、断电不丢设置、连续运行不死机**。

---

## 2. 硬件与接线（速查）

| 模块 | 推荐型号 | 接口 / 地址 | 关键引脚 |
| --- | --- | --- | --- |
| 主控 | ESP32-S3-WROOM-1 **N16R8** | 16 MB Flash + 8 MB PSRAM | — |
| 六轴 IMU | MPU6050（GY-521） | I2C `0x68` | SDA=GPIO8，SCL=GPIO9 |
| PPG | MAX30102 | I2C `0x57` | 同上 |
| 温度 | TMP117 | I2C `0x48` | 同上 |
| 触摸 | FT6236（文档）/ GT9xxx（实物屏） | I2C `0x38` / `0x14`、`0x5D` | INT=17，RST=18 |
| 显示 | 4.3" 800×480 | 文档按 SPI；实物套装为 RGB | SCK=12，MOSI=11，CS=10，DC=14，RST=15，BL=16 |
| 按键 | 6×6 轻触 | GPIO 输入 | GPIO4，10 kΩ 上拉，100 nF |
| 马达 | 0830 + 2N7002 + 1N4148 | GPIO PWM | GPIO5，4 kHz |
| 供电 | USB-C 5 V → 3.3 V/2 A | 不配电池 | 电量固定 100% |

> ⚠️ **屏幕接口先定稿再接线**：文档正文按 SPI 屏写，随附资料 `4.3寸屏-RGB4342-RGB4384` 与 LVGL 例程是 RGB 屏 + GT9xxx。两套的引脚、驱动、刷新带宽完全不同。详见 [`docs/pinout.md`](docs/pinout.md)。

完整引脚表、电源预算与上电检查见 [`docs/pinout.md`](docs/pinout.md)。

---

## 3. 软件架构

### 3.1 分层

```text
┌──────────────────────────────────────────────┐
│ hb_app      AppController / ScreenManager    │  只做编排与页面栈
├──────────────────────────────────────────────┤
│ hb_ui       theme / fonts / widgets          │  唯一允许依赖 LVGL 的层
├──────────────────────────────────────────────┤
│ hb_service  Step / Sedentary / Vital / ...   │  纯逻辑，可主机测试
├──────────────────────────────────────────────┤
│ hb_driver   MPU6050 / MAX30102 / TMP117 ...  │  寄存器与量纲转换
├──────────────────────────────────────────────┤
│ hb_hal      I2CBus / SpiBus / Gpio / Ledc    │  总线、互斥、重试
├──────────────────────────────────────────────┤
│ hb_common   DTO / Event / Settings / 接口    │  无硬件依赖，全层可用
└──────────────────────────────────────────────┘
```

依赖只允许**自上而下**：

| 组件 | 允许依赖 | 禁止 |
| --- | --- | --- |
| `hb_common` | 仅 STL | `esp_*`、`lv_*` |
| `hb_hal` | `hb_common` + ESP-IDF | LVGL、`hb_service` |
| `hb_driver` | `hb_hal`、`hb_common` | LVGL、`hb_service` |
| `hb_service` | `hb_common` + STL | 具体驱动类、`esp_*`、`lv_*` |
| `hb_ui` | `hb_common` + LVGL | 驱动、服务 |
| `hb_app` | `hb_service`、`hb_ui`、`hb_driver` | — |

三条硬规则：

1. `hb_ui` 里不允许出现 `i2c_`、`esp_` 开头的 API。
2. `hb_driver` 里不允许出现 `lv_` 开头的 API。
3. `hb_service` 里不允许直接操作 GPIO，也不允许 `#include "driver/..."`。

提交前跑一遍：

```bash
python tools/check_layers.py
```

> 组件名为什么都带 `hb_`？因为 ESP-IDF 自带 `hal` 和 `driver` 组件，同名会遮蔽官方组件，导致 `driver/i2c_master.h` 直接找不到。

### 3.2 目录

```text
task/
├── CMakeLists.txt              # 双模式入口：TASK_HOST / ESP-IDF
├── CMakePresets.json           # VS Code 与 CLion 的共同契约
├── sdkconfig.defaults
├── partitions.csv              # 双 OTA + LittleFS
├── components/
│   ├── hb_common/              # 值类型、事件、设置、接口
│   ├── hb_hal/                 # I2C 总线（单例 + 锁 + 重试）
│   ├── hb_driver/              # 传感器驱动
│   ├── hb_service/             # 算法与业务
│   ├── hb_ui/                  # 主题与字体（P4）
│   └── hb_app/                 # 编排与页面（P4）
├── main/                       # app_main + 组件装配
├── tests/
│   ├── host/                   # 主机侧测试（不依赖 IDF）
│   └── data/                   # CSV 回放数据
├── tools/                      # 分层检查、CSV 转换、回放
├── docs/                       # 架构、引脚、协议、Git 手册
└── .github/workflows/          # GitHub Actions：主机测试
```

---

## 4. 环境准备

### 4.1 必装

| 工具 | 版本 | 用途 |
| --- | --- | --- |
| Git | 2.30+ | 版本管理 |
| CMake | 3.23+ | 主机测试与 IDE |
| Ninja | 1.10+ | 构建（CLion / ESP-IDF 自带） |
| 主机 C++ 编译器 | 支持 C++17 | GCC 9+ / Clang 10+ / MSVC 2019+ |
| Python | 3.10+ | 工具脚本（可选） |

Windows 上没有主机编译器时，任选其一：

- 安装 **CLion**，使用它自带的 MinGW（最省事）；
- 安装 **MSYS2**，`pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`；
- 安装 **Visual Studio Build Tools**，勾选“使用 C++ 的桌面开发”。

### 4.2 ESP-IDF

推荐 **ESP-IDF 5.3.x**（`工具/esp-idf-tools-setup-offline-5.3.5.exe` 可离线安装）。

安装后在 PowerShell 里执行一次（路径按实际安装位置）：

```powershell
C:\Espressif\frameworks\esp-idf-v5.3.5\export.ps1
idf.py --version
```

每次新开终端都要先 `export.ps1`，否则会报 `IDF_PATH not set`。

---

## 5. 快速开始

### 5.1 只跑主机测试（不需要开发板）

```bash
cmake --preset host-debug
cmake --build --preset host-debug --parallel
ctest --preset host-debug --output-on-failure
```

预期输出：

```text
100% tests passed, 0 tests failed out of 1
```

如果 preset 找不到生成器或编译器，直接指定：

```bash
cmake -S . -B build/host -DTASK_HOST=ON -G "MinGW Makefiles"
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

### 5.2 固件构建与烧录

```powershell
# 1) 进入 ESP-IDF 环境
C:\Espressif\frameworks\esp-idf-v5.3.5\export.ps1

# 2) 首次配置目标芯片
idf.py -B build/idf set-target esp32s3

# 3) 构建
idf.py -B build/idf build

# 4) 烧录 + 串口（把 COM5 换成你的端口）
idf.py -B build/idf -p COM5 flash monitor
```

退出监视器：`Ctrl + ]`。

查看串口列表：

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description
```

> 目前固件只做初始化与空循环，屏幕/传感器任务在 P1–P3 逐步接入。

---

## 6. IDE 配置

### 6.1 VS Code

1. 安装推荐扩展（打开项目后会提示）：`C/C++`、`CMake Tools`、`ESP-IDF`。
2. `File → Open Folder` 选择 `task/`。
3. 命令面板执行 `CMake: Select Configure Preset`，选 **host-debug**。
4. `Terminal → Run Task` 里有：

| 任务 | 作用 |
| --- | --- |
| `host: configure` | 生成 `build/host` |
| `host: build` | 编译主机测试 |
| `host: test` | 运行 CTest |
| `idf: build` | `idf.py build` |
| `idf: flash` | 烧录（会提示输入 COM 口） |
| `idf: monitor` | 串口监视 |

5. 调试主机测试：`Run and Debug → Host tests (gdb)`。

固件相关的编译/烧录建议在 **ESP-IDF 终端**里做，插件只负责语法补全与索引，避免两套环境互相覆盖 `build/`。

### 6.2 CLion

1. `File → Open`，选择 `task/CMakeLists.txt`，以 **Project** 方式打开。
2. `Settings → Build, Execution, Deployment → CMake`：
   - 新增 Profile，Name 填 `host-debug`；
   - Build type 选 `Debug`；
   - CMake options 填 `-DTASK_HOST=ON`；
   - Generator 选 `Ninja`，Toolchain 选 CLion 自带 MinGW（Windows）或系统 GCC。
3. 打开 `tests/host/test_step.cpp`，点行号旁绿色三角即可调试。
4. 固件构建仍用 `idf.py`（CLion 配 xtensa 工具链可以，但不建议在课上折腾）。

`.idea/` 已在 `.gitignore` 中，**不要提交**。团队共享的是 `CMakePresets.json` 和 `.clang-format`，不是某个人的 IDE 配置。

---

## 7. Git 零基础（GitHub）

这一节假设你从没碰过 Git。按顺序做即可。

### 7.1 安装与身份

```bash
git --version
git config --global user.name "你的名字"
git config --global user.email "你的GitHub邮箱"
git config --global init.defaultBranch main
```

`user.email` 建议与 GitHub 账号一致，否则提交记录不会关联到你的头像。

### 7.2 起点 A：仓库已经在 GitHub 上（推荐）

```bash
cd C:\Users\你的名字\Desktop
git clone https://github.com/<owner>/task.git
cd task
```

如果是私有仓库，第一次会弹出登录窗口：**密码位置填 Personal Access Token（PAT）**，不是 GitHub 登录密码。

生成 PAT：GitHub → 右上角头像 → `Settings` → `Developer settings` → `Personal access tokens` → `Fine-grained tokens` → 勾选目标仓库的 `Contents: Read and write`。

### 7.3 起点 B：本地已有代码，要推到 GitHub

1. 在 GitHub 点 `New repository`，名字填 `task`，**不要**勾选 `Add a README file` / `.gitignore` / `license`（本地已经有了）。
2. 本地执行：

```bash
cd task
git init -b main
git add .
git commit -m "chore: 初始化项目骨架"
git remote add origin https://github.com/<你的用户名>/task.git
git push -u origin main
```

3. 把队友加成协作者：仓库 → `Settings` → `Collaborators` → `Add people`。

### 7.4 每天固定动作

```bash
git switch main
git pull --rebase                 # 先同步，避免最后一天大冲突
git switch -c feature/step-tuning # 开自己的分支
# ...改代码、跑测试...
git status                        # 看改了哪些文件
git diff                          # 看具体改了什么
git add components/hb_service/src/step_service.cpp
git commit -m "feat(step): 加入动态阈值与峰谷对"
git push -u origin feature/step-tuning
```

然后在 GitHub 页面点 `Compare & pull request`，填标题和说明，指定一名队友 Review，合并后删掉远程分支。

### 7.5 分支与 PR 规则

| 分支 | 用途 | 示例 |
| --- | --- | --- |
| `main` | 随时可编译、可演示 | 受保护，不允许直接 push |
| `feature/*` | 新功能 | `feature/sedentary-fsm` |
| `fix/*` | 修缺陷 | `fix/i2c-nack-retry` |
| `docs/*` | 文档 | `docs/pinout` |

保护 `main`：仓库 → `Settings` → `Branches` → `Add branch protection rule` → 勾选 `Require a pull request before merging` 和 `Require approvals (1)`。

### 7.6 冲突怎么处理

执行 `git pull --rebase` 后如果提示 `CONFLICT`，文件里会出现：

```text
<<<<<<< HEAD
你的改动
=======
队友的改动
>>>>>>> origin/main
```

处理步骤：

1. 打开文件，保留正确的内容，**删掉三行标记**；
2. `git add <冲突文件>`；
3. `git rebase --continue`；
4. 想放弃这次 rebase：`git rebase --abort`。

### 7.7 撤销操作

| 场景 | 命令 |
| --- | --- |
| 改坏了，还没 `add` | `git restore <file>` |
| 已 `add`，想取消暂存 | `git restore --staged <file>` |
| 刚 commit，还没 push | `git reset --soft HEAD~1` |
| 已经 push，想反向提交 | `git revert <commit>` |
| 改到一半要切分支 | `git stash` → 切分支 → `git stash pop` |
| 找“消失”的提交 | `git reflog` |

### 7.8 常见报错

| 报错 | 原因 | 解决 |
| --- | --- | --- |
| `Authentication failed` | 用了登录密码 | 改用 PAT 或 SSH |
| `rejected ... non-fast-forward` | 远程有你没有的提交 | `git pull --rebase` 后再 push |
| `warning: LF will be replaced by CRLF` | 换行符 | 已由 `.gitattributes` 统一，忽略即可 |
| `file is 120 MB; exceeds GitHub's limit` | 提交了安装包/固件 | 不要提交 `build/`、`工具/`，大文件用 Release |
| `You are in 'detached HEAD' state` | 直接 checkout 了某个提交 | `git switch main` |
| `fatal: not a git repository` | 不在项目目录 | `cd task` |

### 7.9 三条红线

1. 不提交 `build/`、`sdkconfig`、`.idea/`、`managed_components/`。
2. 不对 `main` 执行 `git push --force`。
3. 不把 token、Wi-Fi 密码、个人学号写进仓库。

更完整的场景手册见 [`docs/git.md`](docs/git.md)。

---

## 8. 团队分工

> 下面是 6 个角色的完整定义。3～4 人组按“角色 2+3 合并、角色 5+6 合并”压缩；6 人组一人一岗。

### 角色 1：项目经理 / 系统集成

- **核心职责**：把控整体进度（P0–P7 阶段）、统筹 Git 仓库管理（如分支合并与代码 Review）、负责最终的系统联调。
- **具体任务**：
  - 组织需求评审；
  - 解决跨模块的冲突；
  - 负责最终答辩 PPT 制作与主讲；
  - 撰写总体架构设计文档。

### 角色 2：硬件与底层驱动（硬件/驱动负责人）

- **核心职责**：负责物理连接、电源安全与底层 HAL（硬件抽象层）及 Driver（驱动层）。
- **具体任务**：
  - 接线与排障（I2C、SPI 总线，电源 3.3 V 测量）；
  - 实现 I2CBus 单例加锁、屏幕点屏与触摸校准；
  - 编写 MPU6050、MAX30102、TMP117 的寄存器配置与突发读取代码。

### 角色 3：核心算法（计步与久坐服务）

- **核心职责**：负责文档中“必做”的核心业务逻辑（Service 层）。
- **具体任务**：
  - 实现 StepService：去直流、低通滤波、动态阈值、峰谷检测；
  - 实现 SedentaryService：久坐状态机（6 种状态迁移）、免打扰逻辑、稍后提醒逻辑；
  - 使用 CSV 数据回放调参，保证计步误计率 <3%，久坐误差 ≤10 s。

### 角色 4：LVGL 界面与交互（应用层 UI）

- **核心职责**：负责手环端的 UI 呈现与用户交互（App 层）。
- **具体任务**：
  - 实现六个页面（表盘、运动、心率、体温、久坐、设置）的 UI 布局；
  - 开发弹窗组件（久坐提醒、跌倒确认）与页面切换动画；
  - 实现触摸坐标映射、按键长短按事件、抬腕亮屏逻辑；
  - 提取中文字体子集，优化局部刷新帧率。

### 角色 5：高级功能与数据存储（选做拓展）

- **核心职责**：负责文档中“选做”的生理参数算法，以及数据的本地化存储。
- **具体任务**：
  - （选做）编写 VitalService：PPG 去直流、带通滤波、峰值检测测心率、比值法测血氧；
  - 实现 RecordStore：NVS 设置持久化、LittleFS 分钟级记录（16 字节定长结构写入）；
  - 实现看门狗（Watchdog）喂狗逻辑与异常恢复机制。

### 角色 6：PC 端开发与测试（Qt 与质量保障）

- **核心职责**：负责“选做”的 PC 端上位机开发，以及全系统的测试与文档。
- **具体任务**：
  - （选做）用 Qt5 + SQLite 编写 PC 端数据管理软件（画折线图、柱状图、CSV 导出）；
  - 编写并执行测试用例（单元测试、计步精度对比、连续 8 小时烤机测试）；
  - 负责排障手册编写（表 5-9）、部署手册和用户使用说明。

### 角色与目录对照

| 角色 | 主战场 | 关键交付 |
| --- | --- | --- |
| 1 项目经理 | `docs/`、`main/`、仓库设置 | 架构文档、进度表、答辩 PPT |
| 2 硬件驱动 | `hb_hal/`、`hb_driver/` | 接线验收表、点屏、三传感器读数 |
| 3 核心算法 | `hb_service/`（step、sedentary） | 调参记录、精度数据、状态机 |
| 4 界面交互 | `hb_ui/`、`hb_app/` | 六页面、弹窗、字体子集 |
| 5 高级与存储 | `hb_service/`（vital、record） | PPG 精度表、LittleFS 记录、看门狗 |
| 6 PC 与测试 | `tests/`、`tools/`、Qt 仓库 | 测试报告、排障手册、CSV 工具 |

---

## 9. 工作流程

### 9.1 两个循环

**内环（每次改代码，1 分钟内）：**

```text
改代码 → cmake --build --preset host-debug → ctest → 通过则继续
```

**外环（每个功能，半天到两天）：**

```text
Issue → 分支 → 小步提交 → push → PR → 队友 Review → 合并 → 删分支
```

### 9.2 提交信息

格式：`type(scope): 一句话说明`

| type | 用在哪 |
| --- | --- |
| `feat` | 新功能 |
| `fix` | 修缺陷 |
| `refactor` | 不改行为的重构 |
| `test` | 测试 |
| `docs` | 文档 |
| `chore` | 构建、依赖、杂务 |

正例：`fix(step): 马达震动期间暂停计步`、`docs(pinout): 补充 RGB 屏接口差异`
反例：`update`、`改了一下`、`final`、`aaa`

### 9.3 PR 检查清单

- [ ] `ctest` 全绿；
- [ ] `python tools/check_layers.py` 通过；
- [ ] 没有提交 `build/`、`sdkconfig`、`.idea/`；
- [ ] 新增/修改的公开接口有注释；
- [ ] 硬件相关改动附串口日志或照片；
- [ ] 关联了 Issue 或阶段任务。

### 9.4 阶段计划

| 阶段 | 学时 | 主要任务 | 产出 / 目录 |
| --- | --- | --- | --- |
| P0 | 4 | 需求、分组、仓库 | 需求规格、Issue 列表 |
| P1 | 8 | 环境、点屏、接线 | 可烧录样机、`docs/pinout.md` 核对 |
| P2 | 12 | I2C、三传感器、自检 | `hb_hal/`、`hb_driver/`、`hb_service/sensor_hub` |
| P3 | 14 | 计步、久坐、提醒 | `hb_service/step*`、`sedentary*`、测试数据 |
| P4 | 12 | 六页面、触摸、字体 | `hb_ui/`、`hb_app/screens/` |
| P5 | 12 | 设置、存储、看门狗 | `hb_service/record_store`、NVS |
| P6 | 10 | 联调、故障演练、Qt | 测试报告、PC 端仓库 |
| P7 | 8 | 答辩、互评、复盘 | PPT、视频、改进清单 |

### 9.5 完成定义（DoD）

一个任务只有同时满足以下条件才算“完成”：

1. 代码能编译（主机 + IDF 至少一边，另一边有明确说明）；
2. 有对应测试或可复现的验收步骤；
3. 日志/串口输出能证明现象；
4. 文档或注释同步更新；
5. 经过至少一名队友 Review。

---

## 10. 测试

### 10.1 主机侧

```bash
ctest --preset host-debug --output-on-failure
```

当前覆盖：

| 测试 | 验证点 |
| --- | --- |
| `event_bus` | 按类型分发、退订后不再收到 |
| `step` | 合成步态计数、马达静默期不计数 |
| `sedentary` | 45 min 触发、稍后 10 min、30 步复位、免打扰 |

新增测试：在 `tests/host/` 下建 `test_xxx.cpp`，加进 `tests/host/CMakeLists.txt` 的 `task_tests` 源文件列表，用 `TEST_CASE(name) { CHECK(...); }`。

### 10.2 硬件侧

| 项目 | 方法 | 通过标准 |
| --- | --- | --- |
| 电源 | 万用表 | 3.25–3.35 V，纹波 < 50 mV |
| I2C | `i2c-tools` 扫描 | 0x38/0x48/0x57/0x68 全部出现 |
| 显示 | 点屏程序 | 纯色/彩条无花屏 |
| 触摸 | 五点校准 | 误差 ≤ 2 mm |
| 计步 | 100 步 × 5 组 | 误差 ≤ 3% |
| 久坐 | 静坐 45 min | 误差 ≤ 10 s |
| 稳定性 | 8 h 连续运行 | 不死机、可恢复 |

---

## 11. 排障

| 现象 | 先查什么 |
| --- | --- |
| `cmake: command not found` | 装 CMake 或用 ESP-IDF 终端；CLion 用内置 CMake |
| 找不到 C++ 编译器 | 装 MinGW/MSVC，或在 CLion 里选 Toolchain |
| `IDF_PATH not set` | 先执行 `export.ps1` |
| `CMake Error: could not find generator Ninja` | `pip install ninja` 或改用 `-G "MinGW Makefiles"` |
| 串口打不开 / `Permission denied` | 关掉其他监视器，换 USB 口，检查 CH340 驱动 |
| 屏幕白屏 | 先量 3.3 V 与 BL，再查 CS/DC/RST 与 SPI 模式 |
| I2C 扫不到 | 量上拉、确认 AD0/ADD0、换杜邦线 |
| CLion 头文件标红 | 重新 Load CMake Project；确认 `TASK_HOST=ON` |
| 主机测试乱码 | 终端改 UTF-8；源码本身是 UTF-8 |

---

## 12. 路线图

- [x] 双模式 CMake 骨架、分层检查、GitHub Actions
- [x] `hb_common` 值类型与事件
- [x] `I2CBus` 单例、MPU6050/MAX30102/TMP117 骨架
- [x] `EventBus`、`StepService`、`SedentaryService` + 单元测试
- [ ] P1：显示/触摸后端（SPI 与 RGB 二选一或并存）
- [ ] P2：`SensorHub` 任务与队列、自检页
- [ ] P3：CSV 回放调参、`tools/replay.py` 接串口
- [ ] P4：LVGL 六页面、字体子集、弹窗
- [ ] P5：`RecordStore`、NVS、看门狗、断点续传
- [ ] P6：Qt 上位机、SQLite、CSV 导出
- [ ] P7：部署脚本、答辩材料

---

## 13. 许可与声明

- 本项目仅用于教学，**不是医疗器械**；心率、血氧、体温数据不得用于诊断。
- 第三方组件：ESP-IDF（Apache-2.0）、LVGL（MIT）、FreeRTOS（MIT）、Qt（LGPL，动态链接）。
- 学生代码如引用开源实现，请在 `docs/` 中注明来源与许可证。
