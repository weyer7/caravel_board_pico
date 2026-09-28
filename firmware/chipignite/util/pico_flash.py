#!/usr/bin/env python3

import argparse
import subprocess
import sys
import time


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

    run([
        "picotool",
        "load",
        "-f",
        "-vx",
        uf2,
    ])


def wait_for_usb(timeout=10.0):
    """
    Wait until picotool can see a Pico again.
    """

    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        result = subprocess.run(
            ["picotool", "info"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        if result.returncode == 0:
            return

        time.sleep(0.1)

    raise RuntimeError(
        "Timed out waiting for Pico USB device"
    )


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