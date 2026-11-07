#pragma once

namespace hb {

/// 运行参数。默认值与文档表 3-4 / 4.6 一致，后续由 NVS 覆盖。
struct Settings {
    int thresholdMin = 45;    // 久坐阈值（分钟）
    int snoozeMin = 10;       // 稍后间隔（分钟）
    int maxSnooze = 3;        // 最多稍后次数
    int stepsPer5Min = 30;    // 有效活动：5 分钟内步数
    int quietStartHour = 22;  // 免打扰开始
    int quietEndHour = 7;     // 免打扰结束
    int brightness = 80;      // 背光百分比
    int vibrationLevel = 2;   // 震动强度 0–3
    bool wristRaise = true;   // 抬腕亮屏

    // 算法参数（调参时改这里，不要写死在实现里）
    float stepPeakG = 0.6f;   // 峰谷差阈值（g）
    int sampleRateHz = 50;    // IMU 采样率
};

}  // namespace hb
