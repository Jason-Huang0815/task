# 生成界面用到的中文子集字体（5 个），输出到 components/hb_ui/src/fonts/。
#
# 依赖：Node.js + lv_font_conv
#     npm install -g lv_font_conv@1.5.3
#
# 用法：
#     pwsh -File tools/gen_font_subset.ps1
#     pwsh -File tools/gen_font_subset.ps1 -OutDir D:\tmp\fonts -CnFont C:\Windows\Fonts\msyh.ttc
#
# 说明：
#   * 源字体默认用 Windows 自带的「等线」(Deng.ttf)，它是单体 TTF；
#     msyh.ttc 是 TTC 集合，lv_font_conv 读不了（会报 Unsupported OpenType signature ttcf）。
#   * 字符表来自 tools/font_symbols.txt，改了界面文案要同步这个文件再重跑。
param(
    [string]$OutDir = "",
    [string]$CnFont = "C:\Windows\Fonts\Deng.ttf",
    [string]$NumFont = "C:\Windows\Fonts\consola.ttf",
    [string]$SymbolFile = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $repoRoot "components\hb_ui\src\fonts" }
if ([string]::IsNullOrWhiteSpace($SymbolFile)) { $SymbolFile = Join-Path $PSScriptRoot "font_symbols.txt" }

foreach ($f in @($CnFont, $NumFont, $SymbolFile)) {
    if (-not (Test-Path -LiteralPath $f)) { throw "找不到输入文件：$f" }
}

$lvFontConv = (Get-Command lv_font_conv -ErrorAction SilentlyContinue)
if ($null -eq $lvFontConv) {
    throw "没有找到 lv_font_conv，请先执行：npm install -g lv_font_conv@1.5.3"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# 字符表：去掉空白与重复，保持首次出现顺序
$raw = Get-Content -LiteralPath $SymbolFile -Raw -Encoding UTF8
$seen = [System.Collections.Generic.HashSet[char]]::new()
$chars = [System.Collections.Generic.List[char]]::new()
foreach ($ch in $raw.ToCharArray()) {
    if ([char]::IsWhiteSpace($ch)) { continue }
    if ($seen.Add($ch)) { $chars.Add($ch) }
}
$symbols = -join $chars
Write-Host ("字符表：{0} 个唯一字符" -f $chars.Count)

$numberSymbols = "0123456789:./-%°℃ "

function Invoke-LvFontConv {
    param([string]$Font, [int]$Size, [int]$Bpp, [string]$Name, [string]$File)
    $out = Join-Path $OutDir $File
    $args = @(
        "--font", $Font,
        "--size", "$Size",
        "--bpp", "$Bpp",
        "--format", "lvgl",
        "--no-compress",
        "--lv-font-name", $Name,
        "--lv-include", "lvgl.h",
        "-r", "0x20-0x7E",
        "--symbols", $symbols,
        "-o", $out
    )
    & lv_font_conv @args
    if ($LASTEXITCODE -ne 0) { throw "生成 $Name 失败（exit $LASTEXITCODE）" }
    $kb = [math]::Round((Get-Item -LiteralPath $out).Length / 1KB, 1)
    Write-Host ("  {0,-18} {1,4} px  bpp{2}  {3,7} KB" -f $Name, $Size, $Bpp, $kb)
}

function Invoke-LvFontConvNumbers {
    param([string]$Font, [int]$Size, [int]$Bpp, [string]$Name, [string]$File)
    $out = Join-Path $OutDir $File
    $args = @(
        "--font", $Font,
        "--size", "$Size",
        "--bpp", "$Bpp",
        "--format", "lvgl",
        "--no-compress",
        "--lv-font-name", $Name,
        "--lv-include", "lvgl.h",
        "--symbols", $numberSymbols,
        "-o", $out
    )
    & lv_font_conv @args
    if ($LASTEXITCODE -ne 0) { throw "生成 $Name 失败（exit $LASTEXITCODE）" }
    $kb = [math]::Round((Get-Item -LiteralPath $out).Length / 1KB, 1)
    Write-Host ("  {0,-18} {1,4} px  bpp{2}  {3,7} KB" -f $Name, $Size, $Bpp, $kb)
}

Write-Host "输出目录：$OutDir"
Invoke-LvFontConv -Font $CnFont -Size 16 -Bpp 4 -Name "hb_font_cn_16" -File "hb_font_cn_16.c"
Invoke-LvFontConv -Font $CnFont -Size 20 -Bpp 4 -Name "hb_font_cn_20" -File "hb_font_cn_20.c"
Invoke-LvFontConv -Font $CnFont -Size 26 -Bpp 4 -Name "hb_font_cn_26" -File "hb_font_cn_26.c"
Invoke-LvFontConvNumbers -Font $NumFont -Size 48 -Bpp 4 -Name "hb_font_num_48" -File "hb_font_num_48.c"
Invoke-LvFontConvNumbers -Font $NumFont -Size 112 -Bpp 2 -Name "hb_font_num_112" -File "hb_font_num_112.c"

$total = (Get-ChildItem -LiteralPath $OutDir -Filter *.c | Measure-Object -Property Length -Sum).Sum
Write-Host ("完成：{0}，合计 {1} KB" -f $OutDir, [math]::Round($total / 1KB, 1))
Write-Host "下一步：重新 cmake 配置（字体是 glob 进来的），并确认 HB_UI_HAS_CN_FONT=1"
