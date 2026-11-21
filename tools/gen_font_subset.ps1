# 生成 LVGL 中文子集字体（16/20/24 px）与数字字体（48 px）
#
# 前提：
#   1. 已安装 Node.js 18+；
#   2. 在线环境执行过一次： npm install -g lv_font_conv
#      （离线环境无法安装，改用 LVGL 内置 Montserrat 先跑通，中文会显示为方框，
#        答辩前再找有网络的机器生成。）
#
# 用法（在 ui_skeleton 目录）：
#   powershell -ExecutionPolicy Bypass -File tools\gen_font_subset.ps1
#   powershell -ExecutionPolicy Bypass -File tools\gen_font_subset.ps1 -Font "C:\Windows\Fonts\msyh.ttc"
#
# 产物写入 components/hb_ui/src/fonts/，然后在 hb_ui/CMakeLists.txt 里取消注释
# 字体源文件那一行，并把 HB_UI_HAS_CN_FONT 置 1。
param(
    [string]$Font = "C:\Windows\Fonts\msyh.ttc",
    [string]$OutDir = "components/hb_ui/src/fonts"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command lv_font_conv -ErrorAction SilentlyContinue)) {
    Write-Error "未找到 lv_font_conv，请先执行: npm install -g lv_font_conv"
}

if (-not (Test-Path $Font)) {
    Write-Error "字体文件不存在: $Font（可换成 simhei.ttf / msyh.ttc）"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$symbolsFile = Join-Path $PSScriptRoot "font_symbols.txt"
$symbols = (Get-Content $symbolsFile -Raw) -replace "\s", ""
Write-Host "字符数: $($symbols.Length)"

$targets = @(
    @{ Name = "font_cn_16"; Size = 16; Extra = @("--symbols", $symbols) },
    @{ Name = "font_cn_20"; Size = 20; Extra = @("--symbols", $symbols) },
    @{ Name = "font_cn_24"; Size = 24; Extra = @("--symbols", $symbols) },
    # 大号数字：ASCII 可打印字符即可，体积很小。
    @{ Name = "font_num_48"; Size = 48; Extra = @("--range", "0x20-0x7F") }
)

foreach ($target in $targets) {
    $outFile = Join-Path $OutDir "$($target.Name).c"
    $args = @(
        "--font", $Font,
        "--bpp", "4",
        "--format", "lvgl",
        "--lv-font-name", $target.Name,
        "--size", "$($target.Size)",
        "-o", $outFile
    ) + $target.Extra
    Write-Host "生成 $($target.Name) ..."
    & lv_font_conv @args
    if ($LASTEXITCODE -ne 0) { throw "lv_font_conv 失败: $($target.Name)" }
}

Write-Host "完成。请检查生成的 .c 文件体积（4 个字体合计应 < 200 KB）。"
