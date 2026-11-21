/**
 * 仿真专用 lv_conf.h —— 只给 sim/ 用，固件里用的是 ESP-IDF 的 menuconfig。
 * 与固件的关键差异只有两条：LV_USE_WINDOWS（有窗口）和 LV_USE_SNAPSHOT（能截图）。
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* 颜色：固件是 RGB565，仿真保持一致，截图才和板子对得上 */
#define LV_COLOR_DEPTH 16

/* 内存：PC 上给宽一点，免得调页面时被 LV_MEM_SIZE 卡住 */
#define LV_MEM_SIZE (512 * 1024U)

/* 日志：出问题时能看到 LVGL 自己的报错 */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* 后端：Win32 窗口 + 鼠标键盘；截图用于场景回归 */
#define LV_USE_OS LV_OS_WINDOWS
#define LV_USE_WINDOWS 1
#define LV_USE_SNAPSHOT 1

/* 布局 */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/* 单片机常用字体（无中文时回落到这里） */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_20

/* 断言：仿真里宁可停下来，也不要带着坏状态跑 */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0

/* 用标准库的 malloc/free，方便在调试器里看堆 */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

#endif /* LV_CONF_H */
