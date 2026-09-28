TOOLCHAIN_PATH=/opt/riscv/bin/

# Set the prefix for `riscvXX-unknown-elf-*`
# On installations using `multilib`, this will be `riscv64` even for compiling to 32-bit targets
TOOLCHAIN_PREFIX=riscv64-unknown-linux-gnu
ARCH=rv32i_zicsr
# ARCH=rv32i

# ---- Test patterns for project raven ----

.SUFFIXES:

PATTERN = load

PICO_DIR := pico_caravel
PICO_BUILD := $(PICO_DIR)/build

PICO_HKFLASH_UF2 := $(PICO_BUILD)/pico_hkflash.uf2
PICO_TEAM_UF2 := $(PICO_BUILD)/pico_team.uf2

PICOTOOL := ~/.pico-sdk/picotool/2.3.1/picotool/picotool


# =================================
# Caravel Firmware
# =================================
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

# =================================
# Pico Bridge Firmware
# =================================

pico-configure:
	cmake -S $(PICO_DIR) -B $(PICO_BUILD) -DPICO_BOARD=pico2

pico-hkflash: pico-configure
	cmake --build $(PICO_BUILD) --target pico_hkflash -j

pico-flash-hkflash: pico-hkflash
	python3 ../util/pico_flash.py flash $(PICO_HKFLASH_UF2)

clean-flash: pico-flash-hkflash
	python3 ../util/pico_flash.py wait
	$(MAKE) flash

# =================================
# Pico Team Firmware
# =================================

pico-team: pico-configure
	cmake --build $(PICO_BUILD) --target pico_team -j

flash-team: clean-flash pico-team
	python3 ../util/pico_flash.py flash $(PICO_TEAM_UF2)

flash-pico: pico-team
	python3 ../util/pico_flash.py flash $(PICO_TEAM_UF2)

	
# ---- Clean ----

clean:
	rm -f *.elf *.hex *.bin *.vvp *.vcd

.PHONY: \
	clean \
	hex \
	flash \
	flash2 \
	show_projectid \
	pico-configure \
	pico-hkflash \
	pico-team \
	pico-flash-hkflash \
	clean-flash \
	flash-team