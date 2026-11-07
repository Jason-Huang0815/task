# hb_service

已实现：

| 文件 | 内容 |
| --- | --- |
| `src/event_bus.cpp` | 同步事件总线，订阅/退订/发布 |
| `src/step_service.cpp` | SVM、低通、5 s 滑窗、动态阈值、峰谷对、马达屏蔽 |
| `src/sedentary_service.cpp` | 6 状态久坐机、免打扰、稍后 3 次、30 步复位 |
| `include/service/sedentary_policy.h` | 策略模式骨架（P3 注入） |

待补（按阶段）：

- P2：`sensor_hub.h/.cpp` —— 采样任务、队列、自检；
- P3：`tools/replay.py` 接串口，用真实 CSV 调参；
- P4：`vital_service.h/.cpp` —— PPG 去直流、带通、峰值、比值法；
- P5：`record_store.h/.cpp` —— LittleFS 16 字节记录 + NVS 游标；
- P5：`sync_client.h/.cpp` —— Wi-Fi + JSON Lines + 指数退避；
- P5：`fall_service`、`wear_service`。

约束：本目录**不允许**出现 `esp_*`、`lv_*`、`driver/`、`hal/` 的 include，
否则主机测试无法编译。时间统一用 `int64_t` 微秒，由调用方传入。
