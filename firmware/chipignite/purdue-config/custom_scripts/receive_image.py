#!/usr/bin/env python3

import argparse
import struct
import sys
import time
import zlib

import serial
import tkinter as tk
from PIL import Image, ImageTk


MAGIC = b"IMG!"
HEADER_FORMAT = "<4sBBHHHI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

VERSION = 1
FORMAT_RGB565 = 1


def read_exact(ser, count):
    data = bytearray()

    while len(data) < count:
        chunk = ser.read(count - len(data))

        if not chunk:
            raise TimeoutError(
                f"Serial timeout while receiving data "
                f"({len(data)}/{count} bytes)"
            )

        data.extend(chunk)

    return bytes(data)


def find_magic(ser):
    print("Waiting for IMG! header...")

    window = bytearray()

    while True:
        b = ser.read(1)

        if not b:
            raise TimeoutError("Timed out waiting for IMG! header")

        window += b

        if len(window) > len(MAGIC):
            del window[0]

        if bytes(window) == MAGIC:
            return MAGIC


def rgb565_to_rgb888(pixel):
    r = (pixel >> 11) & 0x1F
    g = (pixel >> 5) & 0x3F
    b = pixel & 0x1F

    r = (r * 255) // 31
    g = (g * 255) // 63
    b = (b * 255) // 31

    return r, g, b


def main():
    parser = argparse.ArgumentParser(
        description="Receive and reconstruct a Caravel RGB565 image."
    )

    parser.add_argument(
        "-p",
        "--port",
        default="/dev/ttyUSB0",
        help="Serial port",
    )

    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=9600,
        help="Baud rate",
    )

    parser.add_argument(
        "-o",
        "--output",
        default="received.png",
        help="Output PNG",
    )

    parser.add_argument(
        "--scale",
        type=int,
        default=2,
        help="Display scale factor",
    )

    args = parser.parse_args()

    print(f"Opening {args.port} at {args.baud} baud...")

    ser = serial.Serial(
        args.port,
        args.baud,
        timeout=2,
    )

    # Discard anything currently buffered.
    ser.reset_input_buffer()

    # Wait for the firmware's binary header.
    find_magic(ser)

    # We already consumed the first 4 bytes.
    remaining_header = read_exact(
        ser,
        HEADER_SIZE - len(MAGIC),
    )

    header = MAGIC + remaining_header

    (
        magic,
        version,
        image_format,
        reserved,
        width,
        height,
        payload_size,
    ) = struct.unpack(HEADER_FORMAT, header)

    if magic != MAGIC:
        raise RuntimeError("Invalid magic")

    if version != VERSION:
        raise RuntimeError(
            f"Unsupported image version: {version}"
        )

    if image_format != FORMAT_RGB565:
        raise RuntimeError(
            f"Unsupported image format: {image_format}"
        )

    expected_payload_size = width * height * 2

    if payload_size != expected_payload_size:
        raise RuntimeError(
            f"Invalid payload size: {payload_size}, "
            f"expected {expected_payload_size}"
        )

    print()
    print("Image:")
    print(f"  Dimensions : {width} x {height}")
    print(f"  Format     : RGB565")
    print(f"  Pixel data : {payload_size} bytes")
    print()

    # Create image.
    image = Image.new("RGB", (width, height))
    pixels = image.load()

    # Tkinter display.
    root = tk.Tk()
    root.title("Caravel Flash Image Receiver")

    display_width = width * args.scale
    display_height = height * args.scale

    display_image = Image.new(
        "RGB",
        (display_width, display_height),
    )

    photo = ImageTk.PhotoImage(display_image)

    canvas = tk.Canvas(
        root,
        width=display_width,
        height=display_height,
    )

    canvas.pack()

    image_item = canvas.create_image(
        0,
        0,
        anchor=tk.NW,
        image=photo,
    )

    status = tk.Label(root, text="Receiving...")
    status.pack()

    root.update()

    # Receive pixels.
    crc = 0
    received = 0

    # Update the display every 256 pixels.
    update_interval = 256
    next_update = update_interval

    start_time = time.time()

    for pixel_index in range(width * height):

        pixel_bytes = read_exact(ser, 2)

        pixel = (
            pixel_bytes[0]
            | (pixel_bytes[1] << 8)
        )

        rgb = rgb565_to_rgb888(pixel)

        x = pixel_index % width
        y = pixel_index // width

        pixels[x, y] = rgb

        # Update CRC with the original RGB565 bytes.
        crc = zlib.crc32(pixel_bytes, crc)

        received += 2

        if received >= next_update:
            # Scale the image for display.
            display_image = image.resize(
                (display_width, display_height),
                Image.Resampling.NEAREST,
            )

            photo = ImageTk.PhotoImage(display_image)

            canvas.itemconfig(
                image_item,
                image=photo,
            )

            canvas.image = photo

            elapsed = time.time() - start_time

            if elapsed > 0:
                rate = received / elapsed

                status.config(
                    text=(
                        f"Received {received:,} / "
                        f"{payload_size:,} bytes  "
                        f"({100 * received / payload_size:.1f}%)  "
                        f"{rate / 1024:.1f} KiB/s"
                    )
                )

            root.update()

            next_update += update_interval

    # Receive CRC.
    received_crc_bytes = read_exact(ser, 4)

    received_crc = struct.unpack(
        "<I",
        received_crc_bytes,
    )[0]

    crc &= 0xFFFFFFFF

    # Final display update.
    display_image = image.resize(
        (display_width, display_height),
        Image.Resampling.NEAREST,
    )

    photo = ImageTk.PhotoImage(display_image)

    canvas.itemconfig(
        image_item,
        image=photo,
    )

    canvas.image = photo

    elapsed = time.time() - start_time

    print()
    print("Transfer complete.")
    print(f"  Received CRC : 0x{received_crc:08X}")
    print(f"  Calculated   : 0x{crc:08X}")
    print()

    if received_crc == crc:
        print("CRC: PASS")
        status.config(
            text="CRC PASS - image received successfully"
        )
    else:
        print("CRC: FAIL")
        status.config(
            text="CRC FAIL - received image is corrupted"
        )

    print(f"Time: {elapsed:.2f} s")

    # Save reconstructed image regardless of CRC result.
    image.save(args.output)

    print(f"Saved: {args.output}")

    root.mainloop()


if __name__ == "__main__":
    main()