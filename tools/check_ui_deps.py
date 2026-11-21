#!/usr/bin/env python3
"""ui_skeleton 分层与风格自检。

三件事：
1. hb_ui 只能依赖 hb_common 与 lvgl，出现 service/、driver/、hal/、app/、esp_ 即失败；
2. 全部源码不得有 Tab、行尾空格、超过 100 列的行；
3. 界面里出现的中文字符必须都能在 tools/font_symbols.txt 里找到（供 lv_font_conv 子集化）。

另外顺手拦几个 LVGL 9 里已经改名/被本项目禁用的 API，避免仿真垫片覆盖不到。
退出码 0 表示全部通过。
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
HB_UI = ROOT / "components" / "hb_ui"
HB_APP = ROOT / "components" / "hb_app"
TESTS = ROOT / "tests"
SYMBOLS = ROOT / "tools" / "font_symbols.txt"

MAX_LINE = 100
CODE_SUFFIXES = {".h", ".cpp", ".py", ".txt"}
SKIP_DIRS = {"build", ".git", "node_modules", "__pycache__"}

# hb_ui 是纯界面层：不能反向依赖服务、驱动、HAL、应用层，也不能碰 ESP-IDF。
HB_UI_FORBIDDEN = ("esp_", "service/", "driver/", "hal/", "app/")

# 本项目禁用或 LVGL 9 已改名的 API（仿真垫片只覆盖白名单里的那些）。
BANNED_APIS = (
    "lv_chart",
    "lv_slider",
    "lv_style_t",
    "lv_img_",
    "lv_obj_remove_style_all",
    "lv_obj_align_to",
    "lv_pct(",
    "lv_obj_center(",
    "LV_SIZE_CONTENT",
    "lv_label_set_text_fmt",
    "lv_snprintf",
)

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def code_files(base):
    """遍历 base 下的源文件，跳过构建产物目录。"""
    if not base.exists():
        return
    for path in sorted(base.rglob("*")):
        if not path.is_file() or path.suffix not in CODE_SUFFIXES:
            continue
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        yield path


def check_layers(errors):
    """hb_ui 的 include 越层检查。"""
    checked = 0
    for path in code_files(HB_UI):
        checked += 1
        text = path.read_text(encoding="utf-8")
        for target in INCLUDE_RE.findall(text):
            if any(token in target for token in HB_UI_FORBIDDEN):
                errors.append(f"{path}: hb_ui 不允许依赖 {target}")
    return checked


def check_style(errors):
    """Tab、行尾空格、行宽检查。utf-8 优先，失败时退化为 utf-8-sig。"""
    checked = 0
    for base in (HB_UI, HB_APP, TESTS):
        for path in code_files(base):
            checked += 1
            for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                if "\t" in line:
                    errors.append(f"{path}:{number}: 出现 Tab，按仓库规范用 4 空格")
                if line != line.rstrip():
                    errors.append(f"{path}:{number}: 行尾有空白字符")
                if len(line) > MAX_LINE:
                    errors.append(f"{path}:{number}: 行宽 {len(line)} 超过 {MAX_LINE}")
    return checked


def strip_comments(text):
    """去掉注释但保留字符串字面量，供禁用 API 扫描用（注释里提到 API 不算违规）。"""
    output = []
    index = 0
    state = "code"
    while index < len(text):
        char = text[index]
        if state == "code":
            if char == "/" and index + 1 < len(text) and text[index + 1] == "/":
                state = "line"
                index += 2
                continue
            if char == "/" and index + 1 < len(text) and text[index + 1] == "*":
                state = "block"
                index += 2
                continue
            if char == '"':
                state = "string"
            elif char == "'":
                state = "char"
            output.append(char)
            index += 1
        elif state == "char":
            output.append(char)
            if char == "\\" and index + 1 < len(text):
                output.append(text[index + 1])
                index += 2
                continue
            if char == "'":
                state = "code"
            index += 1
        elif state == "line":
            if char == "\n":
                state = "code"
                output.append(char)
            index += 1
        elif state == "block":
            if char == "*" and index + 1 < len(text) and text[index + 1] == "/":
                state = "code"
                output.append(" ")
                index += 2
                continue
            if char == "\n":
                output.append(char)
            index += 1
        else:
            output.append(char)
            if char == "\\" and index + 1 < len(text):
                output.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                state = "code"
            index += 1
    return "".join(output)


def check_banned_apis(errors):
    """禁用 API 扫描。"""
    checked = 0
    for base in (HB_UI, HB_APP):
        for path in code_files(base):
            checked += 1
            text = strip_comments(path.read_text(encoding="utf-8"))
            for api in BANNED_APIS:
                if api in text:
                    errors.append(f"{path}: 使用了禁用 API {api}")
    return checked


def string_literals(text):
    """提取 C/C++ 字符串字面量内容，跳过注释与字符常量。"""
    literals = []
    index = 0
    state = "code"
    buffer = []
    while index < len(text):
        char = text[index]
        if state == "code":
            if char == "/" and index + 1 < len(text) and text[index + 1] == "/":
                state = "line"
                index += 2
                continue
            if char == "/" and index + 1 < len(text) and text[index + 1] == "*":
                state = "block"
                index += 2
                continue
            if char == '"':
                state = "string"
                buffer = []
                index += 1
                continue
            if char == "'":
                state = "char"
                index += 1
                continue
            index += 1
        elif state == "char":
            if char == "\\" and index + 1 < len(text):
                index += 2
                continue
            if char == "'":
                state = "code"
            index += 1
        elif state == "line":
            if char == "\n":
                state = "code"
            index += 1
        elif state == "block":
            if char == "*" and index + 1 < len(text) and text[index + 1] == "/":
                state = "code"
                index += 2
                continue
            index += 1
        else:
            if char == "\\" and index + 1 < len(text):
                buffer.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                literals.append("".join(buffer))
                state = "code"
                index += 1
                continue
            buffer.append(char)
            index += 1
    return literals


def check_font_symbols(errors):
    """界面中文必须全部登记在 font_symbols.txt 里。"""
    if not SYMBOLS.exists():
        errors.append(f"{SYMBOLS}: 缺少字体符号表")
        return 0

    symbols = {char for char in SYMBOLS.read_text(encoding="utf-8") if ord(char) > 127}
    missing = {}
    for base in (HB_UI, HB_APP):
        for path in code_files(base):
            for literal in string_literals(path.read_text(encoding="utf-8")):
                for char in literal:
                    if ord(char) > 127 and char not in symbols:
                        missing.setdefault(char, path)
    for char, path in sorted(missing.items()):
        errors.append(f"{path}: 字符 U+{ord(char):04X}（{char}）未登记到 font_symbols.txt")
    return len(symbols)


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")

    errors = []
    layer_files = check_layers(errors)
    style_files = check_style(errors)
    api_files = check_banned_apis(errors)
    symbol_count = check_font_symbols(errors)

    if errors:
        for message in errors:
            print(f"ERROR: {message}")
        print(f"\n{len(errors)} problem(s) found")
        return 1

    print(f"checked: {layer_files} hb_ui files, {style_files} style files, "
          f"{api_files} api files, {symbol_count} font symbols")
    print("OK: layers and style look clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
