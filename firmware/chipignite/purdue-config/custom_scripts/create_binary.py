# with open("test_pattern.bin", "wb") as f:
#     f.write(bytes(range(256)) * 16)

with open("test_pattern.bin", "wb") as f:
    for i in range(4096):
        f.write(bytes([
            i & 0xff,
            (i >> 8) & 0xff,
            0xAA,
            0x55
        ]))