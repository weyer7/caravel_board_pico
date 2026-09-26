#!/usr/bin/env python3

import sys
import os
import time
import binascii
from caravel.hk import HKSpi, CARAVEL_PASSTHRU, CMD_PROGRAM_PAGE, CMD_READ_LO_SPEED

if len(sys.argv) < 2:
    print("Usage: caravel_hkflash.py <file>")
    sys.exit(1)

file_path = sys.argv[1]
if not os.path.isfile(file_path):
    print("File not found.")
    sys.exit(1)

with HKSpi() as hk:
    hk.claim_pins()
    print("Asserting hardware reset")
    hk.hard_reset_assert()
    time.sleep(0.1)

    print("Power cycling Caravel core (monitoring ADC rail discharge)...")
    # hk.power_cycle_caravel()
    print(f"Rails restored - 3V3: {hk.read_voltage_3v3():.2f}V, 1V8: {hk.read_voltage_1v8():.2f}V")
    time.sleep(0.1)

    hk.identify()
    hk.cpu_reset_hold()
    time.sleep(0.2)

    print("\nResetting Flash...")
    hk.flash_reset()
    hk.flash_identify()
    hk.flash_erase()

    # --- Flashing Loop ---
    buf = bytearray()
    addr = 0
    nbytes = 0
    total_bytes = 0

    with open(file_path, mode='r') as f:
        x = f.readline()
        while x != '':
            if x[0] == '@':
                addr = int(x[1:], 16)
                print(f"Setting address to {hex(addr)}")
            else:
                values = bytearray.fromhex(x.strip())
                buf[nbytes:nbytes] = values
                nbytes += len(values)

            x = f.readline()

            if nbytes >= 256 or (x != '' and x[0] == '@' and nbytes > 0):
                total_bytes += nbytes
                hk.flash_write_enable()
                
                wcmd = bytearray((CARAVEL_PASSTHRU, CMD_PROGRAM_PAGE, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF))
                wcmd.extend(buf[0:256])
                hk.slave.exchange(wcmd)
                
                while hk.is_busy():
                    time.sleep(0.00001)

                print(f"addr {hex(addr)}: flash page write successful")

                if nbytes > 256:
                    buf = buf[256:]
                    addr += 256
                    nbytes -= 256
                else:
                    buf = bytearray()
                    addr += 256
                    nbytes = 0

        if nbytes > 0:
            total_bytes += nbytes
            hk.flash_write_enable()
            wcmd = bytearray((CARAVEL_PASSTHRU, CMD_PROGRAM_PAGE, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF))
            wcmd.extend(buf)
            hk.slave.exchange(wcmd)
            while hk.is_busy():
                time.sleep(0.001)
            print(f"addr {hex(addr)}: flash page write successful")

    print(f"\nTotal bytes written: {total_bytes}")

    # --- Verification Loop ---
    print("\nVerifying...")
    addr = 0
    nbytes = 0

    with open(file_path, mode='r') as f:
        x = f.readline()
        while x != '':
            if x[0] == '@':
                addr = int(x[1:], 16)
            else:
                values = bytearray.fromhex(x.strip())
                buf[nbytes:nbytes] = values
                nbytes += len(values)

            x = f.readline()

            if nbytes >= 256 or (x != '' and x[0] == '@' and nbytes > 0):
                read_cmd = bytearray((CARAVEL_PASSTHRU, CMD_READ_LO_SPEED, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF))
                buf2 = hk.slave.exchange(read_cmd, nbytes)
                
                if buf[:nbytes] == buf2:
                    print(f"addr {hex(addr)}: read compare successful")
                else:
                    print(f"addr {hex(addr)}: *** READ COMPARE FAILED ***")

                if nbytes > 256:
                    buf = buf[256:]
                    addr += 256
                    nbytes -= 256
                else:
                    buf = bytearray()
                    addr += 256
                    nbytes = 0

    print("\nReleasing reset and booting Caravel...")
    hk.cpu_reset_release()
    hk.hard_reset_deassert()
    hk.release_pins()
    print("Done!")