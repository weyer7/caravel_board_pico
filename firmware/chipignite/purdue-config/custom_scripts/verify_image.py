#!/usr/bin/env python3

import struct
import sys
import zlib

from PIL import Image

MAGIC = b"IMG!"
VERSION = 1
FORMAT_RGB565 = 1

HEADER_FORMAT = "<4sBBHHHI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


def rgb565_to_rgb888(pixel):
    r = (pixel >> 11) & 0x1F
    g = (pixel >> 5) & 0x3F
    b = pixel & 0x1F

    r = (r * 255) // 31
    g = (g * 255) // 63
    b = (b * 255) // 31

    return r, g, b


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} original.png my_data.bin")
        sys.exit(1)

    original_path = sys.argv[1]
    binary_path = sys.argv[2]

    # ------------------------------------------------------------
    # Read binary
    # ------------------------------------------------------------

    with open(binary_path, "rb") as f:
        data = f.read()

    print(f"Binary size: {len(data)} bytes")

    if len(data) < HEADER_SIZE + 4:
        raise RuntimeError("Binary is too small")

    header = data[:HEADER_SIZE]

    (
        magic,
        version,
        image_format,
        reserved,
        width,
        height,
        payload_size,
    ) = struct.unpack(HEADER_FORMAT, header)

    print(f"Magic:       {magic!r}")
    print(f"Version:     {version}")
    print(f"Format:      {image_format}")
    print(f"Dimensions:  {width} x {height}")
    print(f"Payload:     {payload_size} bytes")

    if magic != MAGIC:
        raise RuntimeError("BAD MAGIC")

    if version != VERSION:
        raise RuntimeError("BAD VERSION")

    if image_format != FORMAT_RGB565:
        raise RuntimeError("BAD FORMAT")

    expected_payload = width * height * 2

    if payload_size != expected_payload:
        raise RuntimeError(
            f"BAD PAYLOAD SIZE: {payload_size}, "
            f"expected {expected_payload}"
        )

    expected_total = HEADER_SIZE + payload_size + 4

    if len(data) != expected_total:
        raise RuntimeError(
            f"BAD FILE SIZE: {len(data)}, "
            f"expected {expected_total}"
        )

    payload = data[HEADER_SIZE:HEADER_SIZE + payload_size]

    stored_crc = struct.unpack(
        "<I",
        data[HEADER_SIZE + payload_size:]
    )[0]

    calculated_crc = zlib.crc32(payload) & 0xFFFFFFFF

    print(f"Stored CRC:  0x{stored_crc:08X}")
    print(f"Calc CRC:    0x{calculated_crc:08X}")

    if stored_crc != calculated_crc:
        raise RuntimeError("CRC MISMATCH")

    print("Binary CRC:   PASS")

    # ------------------------------------------------------------
    # Reconstruct image
    # ------------------------------------------------------------

    reconstructed = Image.new("RGB", (width, height))
    pixels = reconstructed.load()

    for i in range(width * height):
        lo = payload[2 * i]
        hi = payload[2 * i + 1]

        pixel = lo | (hi << 8)

        x = i % width
        y = i // width

        pixels[x, y] = rgb565_to_rgb888(pixel)

    reconstructed.save("roundtrip.png")

    print("Wrote:        roundtrip.png")

    # ------------------------------------------------------------
    # Compare against source image
    # ------------------------------------------------------------

    original = Image.open(original_path).convert("RGB")

    if original.size != (width, height):
        print(
            f"NOTE: Original is {original.size}, "
            f"binary is {(width, height)}"
        )
        print("This is expected if you requested resizing.")

    # The RGB565 conversion is lossy, so compare the reconstructed
    # image against the RGB565 quantization of the original rather
    # than expecting the PNG pixels to be identical.
    if original.size == (width, height):
        original_pixels = original.load()

        max_error = 0
        total_error = 0
        pixel_count = width * height

        for y in range(height):
            for x in range(width):
                r0, g0, b0 = original_pixels[x, y]
                r1, g1, b1 = pixels[x, y]

                error = max(
                    abs(r0 - r1),
                    abs(g0 - g1),
                    abs(b0 - b1),
                )

                max_error = max(max_error, error)
                total_error += (
                    abs(r0 - r1)
                    + abs(g0 - g1)
                    + abs(b0 - b1)
                )

        mean_error = total_error / (pixel_count * 3)

        print()
        print("Round-trip comparison:")
        print(f"Maximum channel error: {max_error}")
        print(f"Mean channel error:    {mean_error:.3f}")

        if max_error <= 8:
            print("IMAGE CONVERSION: PASS")
        else:
            print("IMAGE CONVERSION: SUSPICIOUS")


if __name__ == "__main__":
    main()