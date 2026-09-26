import serial
import serial.tools.list_ports
import time
import sys

# Caravel Registers & Opcodes
CARAVEL_PASSTHRU     = 0xC4
CARAVEL_STREAM_READ  = 0x40
CARAVEL_STREAM_WRITE = 0x80
CARAVEL_REG_READ     = 0x48
CARAVEL_REG_WRITE    = 0x88

CMD_READ_STATUS      = 0x05
CMD_WRITE_ENABLE     = 0x06
CMD_PROGRAM_PAGE     = 0x02
CMD_ERASE_CHIP       = 0x60
CMD_RESET_CHIP       = 0x99
CMD_JEDEC_DATA       = 0x9F
CMD_READ_LO_SPEED    = 0x03

SR_WIP               = 0x01

class PinProxy:
    def __init__(self, toggle_func):
        self._toggle = toggle_func

    def toggle(self):
        self._toggle()

class HKSpi:
    CMD_SPI_EXCHANGE  = 0x01
    CMD_SET_RSTB      = 0x02
    CMD_SET_PWR_EN    = 0x03
    CMD_READ_ADC_3V3  = 0x04
    CMD_READ_ADC_1V8  = 0x05
    CMD_SET_LED1      = 0x06
    CMD_SET_LED2      = 0x07
    CMD_POWER_CYCLE   = 0x08
    CMD_RELEASE_PINS  = 0x09
    CMD_CLAIM_PINS    = 0x0A

    def __init__(self, port=None, **_kwargs):
        if port is None:
            for p in serial.tools.list_ports.comports():
                if "2e8a" in p.hwid.lower(): # Raspberry Pi VID
                    port = p.device
                    break
            if port is None:
                raise RuntimeError("Error: RP2040 board not found on USB bus!")

        self.ser = serial.Serial(port, baudrate=115200, timeout=2.0)
        time.sleep(0.05)

        self._led1_state = False
        self.led1 = PinProxy(self.toggle_led1)

    def slave_exchange(self, data: bytes, read_len: int = 0) -> bytes:
        tx_data = bytes(data) + (b'\x00' * read_len)
        length = len(tx_data)
        cmd = bytes([self.CMD_SPI_EXCHANGE, (length >> 8) & 0xFF, length & 0xFF]) + tx_data
        self.ser.write(cmd)
        res = self.ser.read(length)
        return res[len(data):] if read_len > 0 else res

    class _SlaveProxy:
        def __init__(self, parent):
            self.p = parent

        def exchange(self, data, read_len=0):
            return self.p.slave_exchange(data, read_len)

        def write(self, data):
            self.p.slave_exchange(data, 0)

    @property
    def slave(self):
        return self._SlaveProxy(self)

    # --- Power & Reset Control ---
    def hard_reset_assert(self):
        self.ser.write(bytes([self.CMD_SET_RSTB, 0x00]))

    def hard_reset_deassert(self):
        self.ser.write(bytes([self.CMD_SET_RSTB, 0x01]))

    def set_power_enable(self, state: bool):
        self.ser.write(bytes([self.CMD_SET_PWR_EN, 1 if state else 0]))

    def power_cycle_caravel(self):
        """Power down, wait for 3V3 and 1V8 ADCs to drop below 100mV, then power back up."""
        self.ser.write(bytes([self.CMD_POWER_CYCLE]))
        self.ser.read(1)

    def read_voltage_3v3(self) -> float:
        self.ser.write(bytes([self.CMD_READ_ADC_3V3]))
        res = self.ser.read(2)
        return int.from_bytes(res, 'big') / 1000.0

    def read_voltage_1v8(self) -> float:
        self.ser.write(bytes([self.CMD_READ_ADC_1V8]))
        res = self.ser.read(2)
        return int.from_bytes(res, 'big') / 1000.0

    def toggle_led1(self):
        self._led1_state = not self._led1_state
        self.ser.write(bytes([self.CMD_SET_LED1, 1 if self._led1_state else 0]))

    # --- Caravel HK SPI Commands ---
    def identify(self):
        print("Caravel data:")
        mfg = self.slave_exchange([CARAVEL_STREAM_READ, 0x01], 2)
        mfg_val = int.from_bytes(mfg, byteorder='big')
        print(f"   mfg        = {mfg_val:04x}")

        product = self.slave_exchange([CARAVEL_REG_READ, 0x03], 1)
        print(f"   product    = {int.from_bytes(product, byteorder='big'):02x}")

        data = self.slave_exchange([CARAVEL_STREAM_READ, 0x04], 4)
        print(f"   project ID = {int.from_bytes(data, byteorder='big'):08x}")

        if mfg_val != 0x0456:
            print("Incorrect MFG value, expected 0x0456.")
            sys.exit(2)

    def get_status(self):
        return int.from_bytes(self.slave_exchange([CARAVEL_PASSTHRU, CMD_READ_STATUS], 1), byteorder='big')

    def is_busy(self):
        return self.get_status() & SR_WIP

    def cpu_reset_hold(self):
        self.slave_exchange([CARAVEL_REG_WRITE, 0x0B, 0x01])

    def cpu_reset_release(self):
        self.slave_exchange([CARAVEL_REG_WRITE, 0x0B, 0x00])

    def flash_reset(self):
        self.slave_exchange([CARAVEL_PASSTHRU, CMD_RESET_CHIP])

    def flash_read_jedec(self):
        return self.slave_exchange([CARAVEL_PASSTHRU, CMD_JEDEC_DATA], 3)

    def flash_identify(self):
        jedec = self.flash_read_jedec()
        print(f"JEDEC = {int.from_bytes(jedec, 'big'):06x}")
        if jedec[0:1] != bytes.fromhex('ef'):
            print("Winbond SRAM not found")
            sys.exit(1)

    def flash_write_enable(self):
        self.slave_exchange([CARAVEL_PASSTHRU, CMD_WRITE_ENABLE])

    def flash_erase(self, wait=True, quiet=False):
        if not quiet: print("Erasing chip...")
        self.flash_write_enable()
        self.slave_exchange([CARAVEL_PASSTHRU, CMD_ERASE_CHIP])
        if wait:
            while self.is_busy():
                time.sleep(0.5)
                if not quiet: print('.', end='', flush=True)
                self.led1.toggle()
            if not quiet: print("\ndone")

    def release_pins(self):
        self.ser.write(bytes([self.CMD_RELEASE_PINS]))
        ack = self.ser.read(1)

        if ack != b'\x01':
            raise RuntimeError(f"Failed to release Caravel pins (ACK={ack.hex()})")

    def claim_pins(self):
        self.ser.write(bytes([self.CMD_CLAIM_PINS]))
        ack = self.ser.read(1)

        if ack != b'\x01':
            raise RuntimeError(f"Failed to claim Caravel pins (ACK={ack.hex()})")

    def close(self):
        self.ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()