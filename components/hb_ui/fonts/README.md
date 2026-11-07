# 字体

P4 阶段用 `lv_font_conv` 只提取界面用到的 300–500 个汉字，生成 16/20/24 px 三档，
总体积控制在 200 KB 以内；数字与英文单独用 Montserrat，保证等宽。

示例命令（在仓库根目录执行）：

```bash
lv_font_conv --font SourceHanSansCN-Regular.otf --size 20 \
  --bpp 4 --format lvgl --no-compress \
  --range 0x20-0x7F --symbols "步数心率血氧体温久坐设置已起身稍后" \
  -o components/hb_ui/fonts/font_cn_20.c
```

编译前检查：所有界面汉字都已加入子集，否则 LVGL 会显示方框。
