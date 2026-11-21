#pragma once

// 屏幕变体与 RGB 时序。默认 ATK-MD0430R-800480（4.3 寸，ST7262，800x480）；
// 若模块是 480x272（ST7282），把 HB_UI_LCD_800480 改成 0 即可。
#ifndef HB_UI_LCD_800480
#define HB_UI_LCD_800480 1
#endif

namespace hb::ui {

#if HB_UI_LCD_800480
// 手册 2.2.2 节时序：PCLK typ 30MHz，这里取 16MHz，
// 降低 PSRAM 带宽占用（帧率仍约 34fps，足够界面使用）。
constexpr int kHres = 800;
constexpr int kVres = 480;
constexpr int kPclkHz = 16 * 1000 * 1000;
constexpr int kHsyncBackPorch = 40;
constexpr int kHsyncFrontPorch = 48;
constexpr int kHsyncPulseWidth = 1;
constexpr int kVsyncBackPorch = 32;
constexpr int kVsyncFrontPorch = 13;
constexpr int kVsyncPulseWidth = 1;
#else
// 手册 2.2.1 节时序：PCLK typ 9MHz。
constexpr int kHres = 480;
constexpr int kVres = 272;
constexpr int kPclkHz = 9 * 1000 * 1000;
constexpr int kHsyncBackPorch = 40;
constexpr int kHsyncFrontPorch = 5;
constexpr int kHsyncPulseWidth = 1;
constexpr int kVsyncBackPorch = 8;
constexpr int kVsyncFrontPorch = 8;
constexpr int kVsyncPulseWidth = 1;
#endif

constexpr int kHtotal = kHres + kHsyncBackPorch + kHsyncFrontPorch + kHsyncPulseWidth;
constexpr int kVtotal = kVres + kVsyncBackPorch + kVsyncFrontPorch + kVsyncPulseWidth;
constexpr int kPixelsPerFrame = kHres * kVres;
constexpr int kBytesPerFrame = kPixelsPerFrame * 2;

// 估算帧率 = PCLK / (Htotal * Vtotal)。
constexpr int kFpsEstimate = kPclkHz / (kHtotal * kVtotal);

// 800x480 一帧 768000 字节，内部 SRAM 放不下，必须走 PSRAM。
constexpr bool kNeedPsram = kBytesPerFrame > 300 * 1024;

}  // namespace hb::ui
