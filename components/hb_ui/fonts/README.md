# hb_ui 字体

本目录存放 `lv_font_conv` 生成的 LVGL 9 字体（`.c` / `.h`），**不要手工编辑**。

## 生成方法

```bash
lv_font_conv --no-compress --format lvgl --size 16 --bpp 4 \
  --font NotoSansSC-Regular.otf -r 0x20-0x7F \
  --symbols "$(cat ../tools/font_symbols.txt | tr -d '\n')" \
  -o hb_font_cn_16.c --lv-include lvgl.h
```

同样参数再生成 20px、26px 三档中文子集，以及 48px、112px 两档等宽数字字体
（数字字体只需 `0-9`、`:`、`.`、`-`、`+`、`%`、`/`）。

## 输出与命名

| 文件 | 字号 | 用途 | C 符号 |
| --- | --- | --- | --- |
| `hb_font_cn_16.c` | 16 | 小字说明 | `hb_font_cn_16` |
| `hb_font_cn_20.c` | 20 | 正文 | `hb_font_cn_20` |
| `hb_font_cn_26.c` | 26 | 标题 | `hb_font_cn_26` |
| `hb_font_num_48.c` | 48 | 中等数值 | `hb_font_num_48` |
| `hb_font_num_112.c` | 112 | 表盘时钟 | `hb_font_num_112` |

- 字库总量控制在 200 KB 以内，只收录 `tools/font_symbols.txt` 里的字符。
- 生成后在 `components/hb_ui/CMakeLists.txt` 里编译这些 `.c`，并在编译选项里加
  `HB_UI_HAS_CN_FONT=1`，`include/ui/fonts.h` 里的访问器即返回上述字体；
  未加该宏时全部退回 `LV_FONT_DEFAULT`，界面仍可运行（中文显示为方块）。
- `font_symbols.txt` 更新后必须重新生成字体，否则新文案会缺字。
