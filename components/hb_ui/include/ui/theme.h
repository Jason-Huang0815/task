#pragma once

#include <cstdint>

// 界面设计令牌：颜色、字号、间距、动画与弹窗时长集中在这里，
// 页面只引用常量，不写字面量，保证 800x480 一套设计稿全局一致。

namespace hb::ui {

// 画布（ATK-MD0430R-800480，见 display_config.h）
constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 480;

// 配色：深色底、低饱和、强对比
constexpr uint32_t kColorBg = 0x0B1220;
constexpr uint32_t kColorCard = 0x152238;
constexpr uint32_t kColorCardBorder = 0x243350;
constexpr uint32_t kColorText = 0xF8FAFC;
constexpr uint32_t kColorTextMuted = 0x94A3B8;
constexpr uint32_t kColorSport = 0x2DD4BF;  // 青绿：运动/正向
constexpr uint32_t kColorHeart = 0xFF5A6E;  // 红：心率/警告/跌倒
constexpr uint32_t kColorTemp = 0xFBBF24;   // 琥珀：体温/久坐

// 排版
constexpr int kTitlePx = 26;
constexpr int kBodyPx = 18;
constexpr int kValueMinPx = 40;
constexpr int kValueMaxPx = 112;

// 触摸目标与按钮
constexpr int kTouchTargetPx = 48;
constexpr int kButtonHeightPx = 52;
constexpr int kButtonRadiusPx = 26;

// 布局
constexpr int kSafePx = 24;        // 安全区
constexpr int kCardRadiusPx = 16;  // 卡片圆角
constexpr int kCardGapPx = 16;     // 卡片间距
constexpr int kStatusBarPx = 52;   // 顶部状态栏高度
constexpr int kDotsBarPx = 40;     // 底部页点区高度
constexpr int kDotPx = 8;          // 页点直径
constexpr int kDotGapPx = 12;      // 页点间距

// 页面内容区：状态栏与页点区之间，页面不得越界
constexpr int kContentTopPx = kStatusBarPx;
constexpr int kContentBottomPx = kScreenHeight - kDotsBarPx;

// 动画
constexpr int kPageAnimMs = 180;

// 弹窗
constexpr int kSnoozeSec = 60;  // 久坐提醒 60 秒后自动稍后
constexpr int kFallSec = 15;    // 跌倒确认 15 秒倒计时
constexpr int kMaskOpa = 153;   // 60% 黑遮罩（255 * 0.6）

}  // namespace hb::ui
