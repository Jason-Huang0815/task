#!/usr/bin/env python3
"""把分钟记录 CSV 转成 16 字节定长二进制（LittleFS 落盘格式）。"""

from __future__ import annotations

import argparse
import csv
import struct
import sys
from pathlib import Path

RECORD = struct.Struct("<IHBBhBBI")


def parse_row(row: dict[str, str]) -> bytes:
    temp = row.get("temp_x100") or row.get("tempX100") or "0"
    sit = row.get("sit_min") or row.get("sitMin") or "0"
    values = (
        int(row["ts"]),
        int(row["steps"]),
        int(row.get("hr", 0)),
        int(row.get("spo2", 0)),
        int(temp),
        int(sit),
        int(row.get("flags", 0)),
        0,
    )
    return RECORD.pack(*values)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="输入 CSV")
    parser.add_argument("output", type=Path, help="输出 .bin")
    args = parser.parse_args()

    if not args.input.is_file():
        print(f"找不到输入文件: {args.input}", file=sys.stderr)
        return 1

    chunks: list[bytes] = []
    with args.input.open("r", encoding="utf-8-sig", newline="") as handle:
        for index, row in enumerate(csv.DictReader(handle), start=2):
            try:
                chunks.append(parse_row(row))
            except (KeyError, ValueError) as exc:
                print(f"第 {index} 行解析失败: {exc}", file=sys.stderr)
                return 1

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(b"".join(chunks))
    print(f"写入 {len(chunks)} 条记录，共 {len(chunks) * 16} 字节 -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
