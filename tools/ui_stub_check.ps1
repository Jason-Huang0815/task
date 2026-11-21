# Compile every LVGL-facing translation unit against the local stub headers.
# Purpose: prove the UI code is syntactically valid and type-correct without
# ESP-IDF or the real LVGL sources. It does NOT prove linking or runtime.
#
# Usage (from the repository root):
#   powershell -ExecutionPolicy Bypass -File tools/ui_stub_check.ps1
#
# Exit code 0 = all translation units compiled with 0 errors and 0 warnings.

param(
    [string]$StubDir = (Join-Path $PSScriptRoot 'ui_stub'),
    [string]$Root = (Split-Path $PSScriptRoot -Parent)
)

$ErrorActionPreference = 'Stop'

$uiSrc = Join-Path $Root 'components/hb_ui/src'
$appSrc = Join-Path $Root 'components/hb_app/src'
$includes = @(
    $StubDir,
    (Join-Path $Root 'components/hb_ui/include'),
    (Join-Path $Root 'components/hb_app/include'),
    (Join-Path $Root 'components/hb_common/include'),
    (Join-Path $Root 'components/hb_service/include'),
    (Join-Path $Root 'components/hb_driver/include')
)

$sources = @()
$sources += Get-ChildItem -Path $uiSrc -Filter *.cpp | Sort-Object Name | ForEach-Object { $_.FullName }
# app_controller.cpp belongs to role A (ESP-IDF dependent); it is copied verbatim, not checked here.
$sources += Get-ChildItem -Path $appSrc -Filter *.cpp |
    Where-Object { $_.Name -ne 'app_controller.cpp' } |
    Sort-Object Name | ForEach-Object { $_.FullName }
$sources += Get-ChildItem -Path (Join-Path $appSrc 'screens') -Filter *.cpp | Sort-Object Name | ForEach-Object { $_.FullName }

Write-Host ("translation units: {0}" -f $sources.Count)

# --- locate MSVC (cl.exe needs the vcvars environment for the CRT headers) ---
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found: $vswhere" }
$vsPath = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio with the C++ toolset not found.' }
$devCmd = Join-Path $vsPath 'Common7/Tools/VsDevCmd.bat'
$envLines = & cmd.exe /c "`"$devCmd`" -arch=x64 -host_arch=x64 >nul && set"
foreach ($line in $envLines) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}

$outDir = Join-Path ([System.IO.Path]::GetTempPath()) ('ui_stub_check_' + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $outDir | Out-Null

$includeArgs = $includes | ForEach-Object { '/I"' + $_ + '"' }
$flags = @('/nologo', '/c', '/std:c++17', '/W4', '/permissive-', '/utf-8', '/EHsc') + $includeArgs

$errorCount = 0
$warningCount = 0
foreach ($src in $sources) {
    $obj = Join-Path $outDir ([System.IO.Path]::GetFileNameWithoutExtension($src) + '.obj')
    $output = & cl.exe @flags "/Fo$obj" $src 2>&1
    $code = $LASTEXITCODE
    foreach ($line in $output) {
        $text = [string]$line
        if ($text -match 'error C\d+') { $errorCount++; Write-Host $text }
        elseif ($text -match 'warning C\d+') { $warningCount++; Write-Host $text }
        elseif ($text -match 'fatal error') { $errorCount++; Write-Host $text }
    }
    if ($code -ne 0) {
        Write-Host ("[FAIL] {0} (exit {1})" -f (Split-Path $src -Leaf), $code)
    }
}

Remove-Item -Recurse -Force $outDir
Write-Host ("errors={0} warnings={1}" -f $errorCount, $warningCount)
if ($errorCount -gt 0 -or $warningCount -gt 0) {
    Write-Host 'RESULT: FAILED'
    exit 1
}
Write-Host 'RESULT: OK (0 error, 0 warning)'
exit 0
