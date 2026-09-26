#!/usr/bin/env python3

import argparse
import struct
import zlib
from pathlib import Path

from PIL import Image


MAGIC = b"IMG!"
VERSION = 1
FORMAT_RGB565 = 1

HEADER_FORMAT = "<4sBBHHHI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB888 to 16-bit RGB565."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F

    return (r5 << 11) | (g6 << 5) | b5


def convert_image(input_path, output_path, width=None, height=None):
    image = Image.open(input_path).convert("RGB")

    original_width, original_height = image.size

    # Resize only if explicitly requested.
    if width is not None or height is not None:
        if width is None:
            width = round(original_width * height / original_height)

        if height is None:
            height = round(original_height * width / original_width)

        image = image.resize((width, height), Image.Resampling.LANCZOS)

    width, height = image.size

    if width > 0xFFFF or height > 0xFFFF:
        raise ValueError("Image dimensions must fit in 16 bits.")

    print(f"Input:     {input_path}")
    print(f"Dimensions: {width} x {height}")
    print(f"Format:     RGB565")

    # Convert image to little-endian RGB565.
    payload = bytearray()

    for y in range(height):
        for x in range(width):
            r, g, b = image.getpixel((x, y))

            pixel = rgb888_to_rgb565(r, g, b)

            payload.append(pixel & 0xFF)
            payload.append((pixel >> 8) & 0xFF)

    payload_size = len(payload)

    # CRC is calculated over pixel data only.
    crc = zlib.crc32(payload) & 0xFFFFFFFF

    # Header:
    #
    #   4 bytes  magic
    #   1 byte   version
    #   1 byte   format
    #   2 bytes  reserved
    #   2 bytes  width
    #   2 bytes  height
    #   4 bytes  payload size
    #
    header = struct.pack(
        HEADER_FORMAT,
        MAGIC,
        VERSION,
        FORMAT_RGB565,
        0,
        width,
        height,
        payload_size,
    )

    with open(output_path, "wb") as f:
        f.write(header)
        f.write(payload)
        f.write(struct.pack("<I", crc))

    total_size = len(header) + payload_size + 4

    print(f"Header:     {len(header)} bytes")
    print(f"Pixel data: {payload_size} bytes")
    print(f"CRC32:      0x{crc:08X}")
    print(f"Total:      {total_size} bytes")
    print(f"Output:     {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Convert an image to the Caravel RGB565 image format."
    )

    parser.add_argument("input", help="Input image")
    parser.add_argument(
        "-o",
        "--output",
        default="my_data.bin",
        help="Output binary (default: my_data.bin)",
    )
    parser.add_argument("--width", type=int)
    parser.add_argument("--height", type=int)

    args = parser.parse_args()

    convert_image(
        Path(args.input),
        Path(args.output),
        args.width,
        args.height,
    )


if __name__ == "__main__":
    main()