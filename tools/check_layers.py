#!/usr/bin/env python3
"""分层依赖检查：把 README 里的三条硬规则变成可执行的检查。"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENTS = ROOT / "components"

INCLUDE_RE = re.compile(r'#\s*include\s+[<"]([^">]+)[">]')

RULES: dict[str, list[str]] = {
    "hb_common": ["esp_", "lvgl", "driver/", "hal/", "service/", "app/", "ui/"],
    "hb_hal": ["lvgl", "service/", "app/", "ui/"],
    "hb_driver": ["lvgl", "service/", "app/", "ui/"],
    "hb_service": ["esp_", "lvgl", "driver/", "hal/", "app/", "ui/"],
    "hb_ui": ["esp_", "service/", "driver/", "hal/", "app/"],
    "hb_app": [],
}

SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".c", ".cc", ".cpp"}


def includes_of(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    return INCLUDE_RE.findall(text)


def main() -> int:
    violations: list[str] = []

    for component, forbidden in RULES.items():
        folder = COMPONENTS / component
        if not folder.is_dir():
            continue
        for path in sorted(folder.rglob("*")):
            if path.suffix not in SOURCE_SUFFIXES:
                continue
            for header in includes_of(path):
                for token in forbidden:
                    if token in header:
                        rel = path.relative_to(ROOT)
                        violations.append(
                            f"{rel}: include \"{header}\" 命中禁用规则 \"{token}\""
                        )

    if violations:
        print("分层检查失败：")
        for item in violations:
            print("  - " + item)
        return 1

    print("分层检查通过：hb_common / hb_hal / hb_driver / hb_service / hb_ui")
    return 0


if __name__ == "__main__":
    sys.exit(main())
