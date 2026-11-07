# screens（P4）

六个页面按 `Screen` 生命周期实现，放到本目录，命名建议：

| 文件 | 页面 | 必做 |
| --- | --- | --- |
| `watch_face.cpp` | 表盘：时间、日期、电量、步数、心率、体温 | ✅ |
| `sport_screen.cpp` | 运动：进度环、距离、热量、7 天柱状图 | ✅ |
| `vital_screen.cpp` | 心率血氧：大数字、PPG 波形、信号质量 | 选做 |
| `temp_screen.cpp` | 体温：当前值、基线偏差、24 h 趋势 | 选做 |
| `sedentary_screen.cpp` | 久坐：圆弧倒计时、已坐时长 | ✅ |
| `settings_screen.cpp` | 设置：亮度、阈值、震动、抬腕 | ✅ |
| `alert_overlay.cpp` | 弹窗：久坐/跌倒 | ✅ |

生命周期：

```cpp
class Screen {
public:
    virtual ~Screen() = default;
    virtual void onCreate(void* root) = 0;  // P4 改为 lv_obj_t*
    virtual void onEnter() = 0;
    virtual void onLeave() = 0;
    virtual void onEvent(const hb::Event& event) = 0;
};
```

约束：

- 所有 LVGL 调用必须在 `ui_task` 中；
- 页面切换用 `lv_scr_load_anim`，180 ms；
- 触摸目标 ≥ 48×48 px，正文 ≥ 18 px；
- 页面对象不销毁，返回时保留滚动位置与测量结果。
