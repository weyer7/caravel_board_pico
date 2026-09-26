import serial

# Update port name to match your OS:
# Windows: 'COM3', 'COM4'
# Linux: '/dev/ttyUSB0' or '/dev/ttyACM0'
# macOS: '/dev/tty.usbserial-XXXX'
PORT = '/dev/ttyUSB0'
BAUD = 9600

try:
    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        print(f"Connected to {PORT} at {BAUD} baud.")
        while True:
            # Read line by line from UART
            line = ser.readline()
            if line:
                print(line.decode('utf-8', errors='replace'), end='')
except KeyboardInterrupt:
    print("\nExiting listener.")
except serial.SerialException as e:
    print(f"Error opening port: {e}")