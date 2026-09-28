#!/usr/bin/env python3

import argparse
import subprocess
import sys
import time

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

import serial.tools.list_ports


def find_picotool():
    # First prefer picotool already available in PATH.
    picotool = shutil.which("picotool")
    if picotool:
        return picotool

    # Pico VS Code extension / SDK locations.
    candidates = [
        Path.home() / ".pico-sdk" / "tools" / "picotool" / "picotool",
        Path.home() / ".pico-sdk" / "picotool" / "2.3.1" / "picotool" / "picotool",
    ]

    for candidate in candidates:
        if candidate.is_file() and candidate.stat().st_mode & 0o111:
            return str(candidate)

    raise RuntimeError(
        "picotool was not found. Add picotool to PATH or install/build "
        "picotool from the Raspberry Pi Pico SDK."
    )


PICOTOOL = find_picotool()

def run(cmd):
    print("+", " ".join(str(x) for x in cmd), flush=True)

    result = subprocess.run(cmd)

    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {result.returncode}"
        )


def flash(uf2):
    """
    Flash a UF2 onto a running Pico or a Pico already in BOOTSEL.

    -f tells picotool to force a compatible running application
    into BOOTSEL first.
    """
    run([PICOTOOL, "load", "-f", "-vx", uf2])


def wait_for_usb(timeout=10.0):
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        for port in serial.tools.list_ports.comports():
            if port.vid == 0x2E8A and port.pid == 0x0009:
                print(f"Found Pico bridge at {port.device}")
                return

        time.sleep(0.1)

    raise RuntimeError("Timed out waiting for Pico USB bridge")


def main():
    parser = argparse.ArgumentParser()

    sub = parser.add_subparsers(
        dest="command",
        required=True,
    )

    p_flash = sub.add_parser("flash")
    p_flash.add_argument("uf2")

    p_wait = sub.add_parser("wait")
    p_wait.add_argument(
        "--timeout",
        type=float,
        default=10.0,
    )

    args = parser.parse_args()

    try:
        if args.command == "flash":
            flash(args.uf2)

        elif args.command == "wait":
            wait_for_usb(args.timeout)

    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())