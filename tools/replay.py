#!/usr/bin/env python3
"""按固定频率回放 IMU CSV：本地打印，或写到串口给固件消费。"""

from __future__ import annotations

import argparse
import csv
import sys
import time
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="IMU CSV")
    parser.add_argument("--port", help="串口，如 COM5；不填则打印到标准输出")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--rate", type=float, default=50.0, help="回放频率 Hz")
    parser.add_argument("--loop", action="store_true", help="循环回放")
    args = parser.parse_args()

    if not args.input.is_file():
        print(f"找不到输入文件: {args.input}", file=sys.stderr)
        return 1

    serial_port = None
    if args.port:
        try:
            import serial  # type: ignore
        except ImportError:
            print("需要 pyserial: pip install pyserial", file=sys.stderr)
            return 1
        serial_port = serial.Serial(args.port, args.baud, timeout=1)

    delay = 1.0 / max(args.rate, 1.0)
    try:
        while True:
            with args.input.open("r", encoding="utf-8-sig", newline="") as handle:
                for row in csv.DictReader(handle):
                    line = ",".join(
                        [
                            row["ts_us"],
                            row["ax"],
                            row["ay"],
                            row["az"],
                            row["gx"],
                            row["gy"],
                            row["gz"],
                        ]
                    )
                    if serial_port is not None:
                        serial_port.write((line + "\n").encode("ascii"))
                    else:
                        print(line)
                    time.sleep(delay)
            if not args.loop:
                break
    except KeyboardInterrupt:
        pass
    finally:
        if serial_port is not None:
            serial_port.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
