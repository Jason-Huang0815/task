# 引脚与接线

> 以实物为准。文档正文按 **SPI 屏 + FT6236** 写；随附的 4.3 寸套装是 **RGB 屏 + GT9xxx**。
> 接线前先确认你手里是哪一套，两套的引脚表不同。

---

## 1. 主控资源

| 资源 | 分配 | 说明 |
| --- | --- | --- |
| 主控 | ESP32-S3-WROOM-1 N16R8 | 16 MB Flash，8 MB Octal PSRAM |
| I2C0 | SDA=GPIO8，SCL=GPIO9 | 400 kHz，4.7 kΩ 上拉 |
| SPI2 | SCK=12，MOSI=11，CS=10，DC=14，RST=15，BL=16 | 仅 SPI 屏方案 |
| 按键 | GPIO4 | 内部/外部 10 kΩ 上拉，低有效 |
| 马达 | GPIO5 | 4 kHz PWM，经 2N7002 低边驱动 |
| 触摸 | INT=17，RST=18 | FT6236（0x38）或 GT9xxx |
| USB | GPIO19/20 | 原生 USB Serial/JTAG |

### 必须避开的引脚

| 引脚 | 原因 |
| --- | --- |
| GPIO0 / 3 / 45 / 46 | 启动 strapping，上电电平决定启动模式 |
| GPIO33–37 | N16R8 的八线 PSRAM 占用 |
| GPIO19 / 20 | 原生 USB |
| GPIO43 / 44 | UART0 日志 |

---

## 2. 方案 A：SPI 屏 + FT6236（文档方案）

| 信号 | 引脚 | 连接对象 | 备注 |
| --- | --- | --- | --- |
| SDA | GPIO8 | MPU6050 / MAX30102 / TMP117 / FT6236 | 共用 I2C0 |
| SCL | GPIO9 | 同上 | 400 kHz |
| SCK | GPIO12 | 屏 SCK | 串联 22 Ω 抑制过冲 |
| MOSI | GPIO11 | 屏 SDA | 单向，无需 MISO |
| CS | GPIO10 | 屏 CS | 低有效 |
| DC | GPIO14 | 屏 DC/RS | 命令/数据 |
| RST | GPIO15 | 屏 RESET | 低有效，上电延时 10 ms |
| BL | GPIO16 | 背光 EN | LEDC 20 kHz |
| INT | GPIO17 | FT6236 INT | 下降沿 |
| RST | GPIO18 | FT6236 RST | 低有效 |

I2C 地址：MPU6050 `0x68`（AD0 接地）、MAX30102 `0x57`、TMP117 `0x48`（ADD0 接地）、FT6236 `0x38`。

---

## 3. 方案 B：RGB 屏 + GT9xxx（随附套装）

- 显示：`esp_lcd_panel_rgb`，需要 HSYNC/VSYNC/DE/PCLK + 16 位数据线，具体引脚以 `DNESP32S3` 原理图与 `23_rgb` 例程为准。
- 触摸：GT9147/GT911/GT1151，地址 `0x14` 或 `0x5D`（由复位时序决定）。
- IO 扩展：XL9555（I2C）负责背光、复位等控制。
- 参考代码：`esp32/1，标准例程-IDF(v5.3.x)/23_rgb`、`esp32/4，扩展例程-IDF版/4，LVGL例程/01_lvgl_transplant`。

> 如果选方案 B，`docs/protocol.md` 与业务代码不受影响，只需要替换 `hb_driver` 的显示/触摸实现，并在 `hb_hal` 增加 RGB 面板初始化。

---

## 4. 电源

| 项目 | 参数 |
| --- | --- |
| 输入 | USB-C 5 V，经 ESD 与 1 A 保险丝 |
| 降压 | MP1584EN → 3.3 V / 2 A（不建议 AMS1117，600 mA 时约 1 W 发热） |
| 典型电流 | 约 250 mA |
| 峰值电流 | 约 600 mA（Wi-Fi + 马达 + 背光） |
| 电池 | 无；电量固定显示 100% |

去耦：3.3 V 入口 100 µF 电解 + 10 µF + 0.1 µF；马达电源单独 100 µF。

---

## 5. 马达驱动

```text
GPIO5 ──1 kΩ──┐
              │ 2N7002 (N-MOS)
3.3 V ── 马达 ── 漏极
              │
GND ────────── 源极
栅极对地 10 kΩ 下拉；马达两端反并联 1N4148（阴极接 3.3 V）
```

PWM 4 kHz。马达远离 IMU，中间加泡棉；软件在震动期间及之后 300 ms 屏蔽计步。

---

## 6. 上电检查清单

1. 断电，万用表蜂鸣档测 3.3 V 与 GND **不短路**；
2. 上电，3.3 V 在 3.25–3.35 V；
3. I2C 扫描依次出现 `0x38`、`0x48`、`0x57`、`0x68`；
4. 背光点亮但无图像 → 查 CS/DC/RST、SPI 模式、字节序；
5. 触摸坐标偏移 → 查分辨率映射与旋转；
6. 马达不震 → 查 MOSFET 方向、PWM 通道、GPIO 复用。
