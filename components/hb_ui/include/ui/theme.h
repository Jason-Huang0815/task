#pragma once

#include <cstdint>

namespace hb::ui {

// 画布
constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 480;

// 配色（文档 3.5.6：深色底、低饱和、强对比）
constexpr uint32_t kColorBg = 0x0B1220;
constexpr uint32_t kColorCard = 0x152238;
constexpr uint32_t kColorText = 0xF8FAFC;
constexpr uint32_t kColorTextMuted = 0x94A3B8;
constexpr uint32_t kColorSport = 0x2DD4BF;   // 青绿：运动/正向
constexpr uint32_t kColorHeart = 0xFF5A6E;   // 红：心率/警告
constexpr uint32_t kColorTemp = 0xFBBF24;    // 琥珀：体温/久坐

// 排版
constexpr int kTitlePx = 26;
constexpr int kBodyPx = 18;
constexpr int kValueMinPx = 40;
constexpr int kValueMaxPx = 112;

// 触摸目标（不小于 48 px）
constexpr int kTouchTargetPx = 48;
constexpr int kButtonHeightPx = 52;

// 动画
constexpr int kPageAnimMs = 180;

}  // namespace hb::ui
