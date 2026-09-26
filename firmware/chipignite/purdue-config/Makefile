TOOLCHAIN_PATH=/opt/riscv/bin/

# Set the prefix for `riscvXX-unknown-elf-*`
# On installations using `multilib`, this will be `riscv64` even for compiling to 32-bit targets
TOOLCHAIN_PREFIX=riscv64-unknown-linux-gnu
ARCH=rv32i_zicsr
# ARCH=rv32i

# ---- Test patterns for project raven ----

.SUFFIXES:

PATTERN = load

hex: ${PATTERN:=.hex}

%.elf: %.c ../sections.lds ../crt0_vex.S
	#$(TOOLCHAIN_PATH)riscv32-unknown-elf-gcc -O0 -march=rv32i -Wl,-Bstatic,-T,../sections.lds,--strip-debug -ffreestanding -nostdlib -o $@ ../start.s ../print_io.c $<
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-gcc -I../ -I../generated/ -O0 -mabi=ilp32 -march=$(ARCH) -D__vexriscv__ -Wl,-Bstatic,-T,../sections.lds,--strip-debug -ffreestanding -nostdlib -o $@ ../crt0_vex.S ../isr.c ../stub.c my_data.S $<
	${TOOLCHAIN_PATH}$(TOOLCHAIN_PREFIX)-objdump -D load.elf > load.lst

%.hex: %.elf
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-objcopy -O verilog $< $@
	sed -ie 's/@1000/@0000/g' $@

%.bin: %.elf
	$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)-objcopy -O binary $< $@

client: client.c
	gcc client.c -o client

flash: load.hex
	python3 ../util/caravel_hkflash.py load.hex

flash2: load.hex
	python3 ../util/caravel_flash.py load.hex

show_projectid:
	python3 ../util/caravel_projectid.py

# ---- Clean ----

clean:
	rm -f *.elf *.hex *.bin *.vvp *.vcd

.PHONY: clean hex all flash