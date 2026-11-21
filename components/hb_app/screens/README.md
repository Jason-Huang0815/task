# screens（P4 · 已落地）

页面按 `hb::ui::Screen` 生命周期实现，全部放在本目录（`components/hb_app/screens/`）。
头文件在 `components/hb_app/include/app/screens/`，实现按同名文件一一对应。

| 文件 | 页面 | 必做 | 状态 |
| --- | --- | --- | --- |
| `watch_face.cpp` | 表盘：时间、日期、电量、步数、心率、体温 | ✅ | 已实现 |
| `sport_screen.cpp` | 运动：进度环、距离、热量、7 天柱状图 | ✅ | 已实现 |
| `vital_screen.cpp` | 心率血氧：大数字、PPG 波形、信号质量 | 选做 | 骨架 + "未启用"占位 |
| `temp_screen.cpp` | 体温：当前值、基线偏差、24 h 趋势 | 选做 | 骨架 + "未启用"占位 |
| `sedentary_screen.cpp` | 久坐：状态、已坐时长、稍后按钮 | ✅ | 已实现 |
| `settings_screen.cpp` | 设置：亮度、阈值、震动、抬腕 | ✅ | 已实现 |
| `alert_overlay.cpp` | 弹窗：久坐提醒 / 跌倒确认 | ✅ | 已实现 |

## 生命周期

```cpp
class Screen {
public:
    virtual ~Screen() = default;
    virtual PageId id() const = 0;
    virtual const char* title() const = 0;
    virtual void onCreate(lv_obj_t* root) = 0;   // 建控件，只做一次
    virtual void onEnter() = 0;                  // 切到本页：请求测量、启动动画
    virtual void onLeave() = 0;                  // 离开本页：停定时器、保存滚动位置
    virtual void onEvent(const hb::Event& event) = 0;  // 仍在 ui_task 线程
    virtual void refresh(const Model& model) = 0;      // 只改会变的字符串
    lv_obj_t* screen() const;                    // 本页独立的 screen 对象
};
```

## 约束

- 所有 LVGL 调用必须在 `ui_task` 中；跨任务数据只经 `UiBridge::onEvent` 进队列。
- 切页走 `compat::loadScreen()`（内部是 LVGL 9 的 `lv_screen_load_anim(..., LV_SCR_LOAD_ANIM_FADE_ON, 180 ms, ...)`）；
  **不要**再直接写 LVGL 8 的 `lv_scr_load_anim`。
- 页面只画 `kContentTopPx .. kContentBottomPx`（状态栏与页点区之间），chrome 由 `UiService::attachChrome()` 统一挂载。
- 触摸目标 ≥ 48×48 px，正文 ≥ 18 px，关键数值 40–112 px。
- 页面对象在创建后**不销毁**，只切换 screen 的显示；返回时保留滚动位置与测量结果。
