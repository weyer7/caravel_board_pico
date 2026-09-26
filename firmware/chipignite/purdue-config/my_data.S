.section .rodata
.global my_flash_binary_start
.global my_flash_binary_end

.align 2

my_flash_binary_start:
.incbin "../../../my_data.bin"
my_flash_binary_end: