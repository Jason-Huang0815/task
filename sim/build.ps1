# 一条命令把界面仿真跑起来：配 LVGL → 编译 → 跑 10 个场景 → 打印截图目录。
#
#   powershell -ExecutionPolicy Bypass -File sim/build.ps1            # 全流程
#   powershell -ExecutionPolicy Bypass -File sim/build.ps1 -Gui       # 编译后直接打开交互窗口
#   powershell -ExecutionPolicy Bypass -File sim/build.ps1 -SkipTests # 只编译
#
# 不需要 ESP-IDF 环境变量：LVGL 源码从 sim/third_party/lvgl 或 managed_components 取。

[CmdletBinding()]
param(
    [switch]$Gui,
    [switch]$SkipTests,
    [string]$Config = "Release",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

$SimDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $SimDir
$BuildDir = Join-Path $SimDir "build"
$ThirdParty = Join-Path $SimDir "third_party\lvgl"

function Write-Step($text) { Write-Host "`n== $text ==" -ForegroundColor Cyan }

# ---------------------------------------------------------------- LVGL 源码
if (-not (Test-Path (Join-Path $ThirdParty "lvgl.h"))) {
    $managed = Join-Path $RepoRoot "managed_components\lvgl__lvgl"
    if (Test-Path (Join-Path $managed "lvgl.h")) {
        Write-Step "从 managed_components 复制一份精简 LVGL 到 sim/third_party/lvgl"
        New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null
        foreach ($item in @("src", "lvgl.h", "lv_version.h", "CMakeLists.txt", "LICENCE.txt", "Kconfig", "README.md")) {
            $src = Join-Path $managed $item
            if (Test-Path $src) { Copy-Item -Recurse -Force $src (Join-Path $ThirdParty $item) }
        }
        $cmakeSrc = Join-Path $managed "env_support\cmake"
        if (Test-Path $cmakeSrc) {
            New-Item -ItemType Directory -Force -Path (Join-Path $ThirdParty "env_support") | Out-Null
            Copy-Item -Recurse -Force $cmakeSrc (Join-Path $ThirdParty "env_support\cmake")
        }
        foreach ($tpl in @("lvgl.pc.in", "lv_version.h.in")) {
            $src = Join-Path $managed $tpl
            if (Test-Path $src) { Copy-Item -Force $src (Join-Path $ThirdParty $tpl) }
        }
        # 精简副本不带官方 demo/example，给 LVGL 的 GLOB 各放一个占位源文件，避免空 target。
        foreach ($dir in @("demos", "examples")) {
            New-Item -ItemType Directory -Force -Path (Join-Path $ThirdParty $dir) | Out-Null
            $stub = Join-Path $ThirdParty "$dir\lv_${dir}_placeholder.c"
            if (-not (Test-Path $stub)) {
                Set-Content -Path $stub -Encoding UTF8 -Value "/* placeholder: 精简副本不带官方 $dir，这里给 CMake GLOB 一个源文件。 */`n#include `"lvgl.h`""
            }
        }
    }
    else {
        Write-Host "没有找到 LVGL 源码。" -ForegroundColor Yellow
        Write-Host "  办法 A：先在仓库根跑一次 idf.py build（组件管理器会拉下 lvgl__lvgl），再重跑本脚本"
        Write-Host "  办法 B：git clone --depth 1 --branch v9.2.2 https://github.com/lvgl/lvgl sim/third_party/lvgl"
        exit 1
    }
}

# ---------------------------------------------------------------- 配置
Write-Step "配置 CMake（Visual Studio 2022 x64）"
cmake -S $SimDir -B $BuildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "cmake 配置失败" }

# ---------------------------------------------------------------- 编译
Write-Step "编译（-j 并行，首次要编 LVGL 三百多个 C 文件）"
$jobs = [Environment]::ProcessorCount
cmake --build $BuildDir --config $Config -j $jobs
if ($LASTEXITCODE -ne 0) { throw "编译失败" }

# ---------------------------------------------------------------- 测试
if (-not $SkipTests) {
    Write-Step "跑场景测试"
    $tests = Join-Path $BuildDir "bin\$Config\ui_sim_tests.exe"
    if (-not $OutDir) { $OutDir = Join-Path $SimDir "out" }
    $env:SIM_OUT_DIR = $OutDir
    & $tests
    if ($LASTEXITCODE -ne 0) { throw "场景测试失败" }
    if ($OutDir) { $shotDir = $OutDir } else { $shotDir = Join-Path $SimDir "out" }
    Write-Host "`n截图：$shotDir" -ForegroundColor Green
}

# ---------------------------------------------------------------- 窗口
if ($Gui) {
    Write-Step "打开交互窗口（1-6 切页 / L 长按 / V 回表盘 / A 久坐 / F 跌倒 / S 步数 / Esc 退出）"
    $app = Join-Path $BuildDir "bin\$Config\ui_sim.exe"
    & $app
}

Write-Host "`n完成。" -ForegroundColor Green
