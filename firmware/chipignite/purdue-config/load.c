#include <defs.h>
#include <stub.h>

//declare external assembly pointers
extern const uint8_t my_flash_binary_start[];
extern const uint8_t my_flash_binary_end[];

#define IMAGE_HEADER_SIZE 16
#define IMAGE_CRC_SIZE    4

// SRAM address space
#define SRAM_BASE ((volatile uint32_t*)0x33000000)
#define MGMT_SRAM_BASE ((volatile uint32_t*)0x00000000)
#define STORAGE_SRAM_BASE ((volatile uint32_t*)0x90000000)

// flash memory address space
#define FLASH_BASE ((volatile uint32_t*)0x10000000)

// --------------------------------------------------------
// Firmware routines
// --------------------------------------------------------

void delay(const int d)
{

    /* Configure timer for a single-shot countdown */
	reg_timer0_config = 0;
	reg_timer0_data = d;
    reg_timer0_config = 1;

    // Loop, waiting for value to reach zero
   reg_timer0_update = 1;  // latch current value
   while (reg_timer0_value > 0) {
           reg_timer0_update = 1;
   }

}

// void stream_binary(void)
// {
//     uint32_t size =
//         (uint32_t)(my_flash_binary_end - my_flash_binary_start);

//     const uint8_t *data = my_flash_binary_start;

//     print("\nStarting binary image stream...\n");
//     print("Address: 0x");
//     print_hex((uint32_t)data, 8);
//     print("\n");
//     print("Size: ");
//     print_dec(size);
//     print(" bytes\n");

//     delay(100000);

//     for (uint32_t i = 0; i < size; i++) {
//         putchar(data[i]);
//     }
// }

void stream_binary(void)
{
    uint32_t size =
        (uint32_t)(my_flash_binary_end - my_flash_binary_start);

    const uint8_t *data = my_flash_binary_start;

    for (uint32_t i = 0; i < size; i++) {
        putchar(data[i]);
    }
}

void print_binary_contents(void) {
    uint32_t size = (uint32_t)(my_flash_binary_end - my_flash_binary_start);
    const uint8_t *bin_ptr = (const uint8_t *)my_flash_binary_start;

    print("\n================ EMBEDDED BINARY DUMP ================\n");
    print("Start Flash Addr : 0x");
    print_hex((uint32_t)bin_ptr, 8);
    print("\n");

    print("End Flash Addr   : 0x");
    print_hex((uint32_t)my_flash_binary_end, 8);
    print("\n");

    print("Total Size       : ");
    print_dec(size);
    print(" bytes\n");

    print("------------------------------------------------------\n");

    for (uint32_t i = 0; i < size; i += 16) {

        uint32_t row_size = size - i;
        if (row_size > 16)
            row_size = 16;

        // Print address
        print("0x");
        print_hex((uint32_t)(bin_ptr + i), 8);
        print(": ");

        // Print hexadecimal bytes
        for (uint32_t j = 0; j < 16; j++) {
            if (j < row_size) {
                print_hex(bin_ptr[i + j], 2);
                print(" ");
            } else {
                // Pad incomplete rows so ASCII stays aligned
                print("   ");
            }
        }

        print("\n     ASCII: ");
        // Print ASCII representation
        for (uint32_t j = 0; j < row_size; j++) {
            uint8_t c = bin_ptr[i + j];
            switch (c) {
                case '\0': print("\\0"); break;
                case '\n': print("\\n"); break;
                case '\r': print("\\r"); break;
                case '\t': print("\\t"); break;
                case '\b': print("\\b"); break;
                case '\f': print("\\f"); break;
                case '\v': print("\\v"); break;
                default:
                    if (c >= 0x20 && c <= 0x7e) {
                        // Printable ASCII
                        putchar(c);
                    } else {
                        // Other non-printable byte
                        print("\\x");
                        print_hex(c, 2);
                    }
                    break;
            }
            // Add spacing between characters to match the byte dump
            print("");
        }
        print("\n");
    }
    print("======================================================\n\n");
}

void test_image_samples(void)
{
    const uint8_t *p = my_flash_binary_start;
    uint32_t size = (uint32_t)(my_flash_binary_end -
                               my_flash_binary_start);

    uint32_t offsets[] = {
        0,
        1,
        2,
        3,
        15,
        16,
        31,
        32,
        255,
        256,
        1023,
        1024,
        4095,
        4096,
        size / 2,
        size - 16
    };

    uint32_t count =
        sizeof(offsets) / sizeof(offsets[0]);

    print("\nIMAGE SAMPLE TEST\n");

    for (uint32_t n = 0; n < count; n++) {
        uint32_t i = offsets[n];

        if (i >= size)
            continue;

        print("Offset 0x");
        print_hex(i, 8);
        print(": ");

        for (uint32_t j = 0; j < 16 && i + j < size; j++) {
            print_hex(p[i + j], 2);
            putchar(' ');
        }

        putchar('\n');
    }
}

void test_pattern(void)
{
    const uint8_t *p = my_flash_binary_start;

    for (uint32_t i = 0; i < 256; i++) {
        print_hex(p[i], 2);
        putchar(' ');

        if ((i & 15) == 15)
            putchar('\n');
    }
}

void configure_io()
{

//  ======= Useful GPIO mode values =============

//      GPIO_MODE_MGMT_STD_INPUT_NOPULL
//      GPIO_MODE_MGMT_STD_INPUT_PULLDOWN
//      GPIO_MODE_MGMT_STD_INPUT_PULLUP
//      GPIO_MODE_MGMT_STD_OUTPUT
//      GPIO_MODE_MGMT_STD_BIDIRECTIONAL
//      GPIO_MODE_MGMT_STD_ANALOG

//      GPIO_MODE_USER_STD_INPUT_NOPULL
//      GPIO_MODE_USER_STD_INPUT_PULLDOWN
//      GPIO_MODE_USER_STD_INPUT_PULLUP
//      GPIO_MODE_USER_STD_OUTPUT
//      GPIO_MODE_USER_STD_BIDIRECTIONAL
//      GPIO_MODE_USER_STD_ANALOG


//  ======= set each IO to the desired configuration =============

    //  GPIO 0 is turned off to prevent toggling the debug pin; For debug, make this an output and
    //  drive it externally to ground.

    reg_mprj_io_0 = GPIO_MODE_MGMT_STD_ANALOG;

    // Changing configuration for IO[1-4] will interfere with programming flash. if you change them,
    // You may need to hold reset while powering up the board and initiating flash to keep the process
    // configuring these IO from their default values.

    // reg_mprj_io_1 = GPIO_MODE_MGMT_STD_OUTPUT;
    // reg_mprj_io_2 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;
    // reg_mprj_io_3 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;
    // reg_mprj_io_4 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;

    reg_mprj_io_1 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
    reg_mprj_io_2 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
    reg_mprj_io_3 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
    reg_mprj_io_4 = GPIO_MODE_USER_STD_BIDIRECTIONAL;

    // -------------------------------------------

    reg_mprj_io_5 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;     // UART Rx
    reg_mprj_io_6 = GPIO_MODE_MGMT_STD_OUTPUT;           // UART Tx
    reg_mprj_io_7 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_8 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_9 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_10 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_11 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_12 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_13 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_14 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_15 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_16 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_17 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_18 = GPIO_MODE_MGMT_STD_OUTPUT;

    reg_mprj_io_19 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_20 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_21 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_22 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_23 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_24 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_25 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_26 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_27 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_28 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_29 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_30 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_31 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_32 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_33 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_34 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_35 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_36 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_37 = GPIO_MODE_MGMT_STD_OUTPUT;

    // Initiate the serial transfer to configure IO
    reg_mprj_xfer = 1;
    while (reg_mprj_xfer == 1);
}

void main()
{
	int i, j, k;

    reg_gpio_mode1 = 1;
    reg_gpio_mode0 = 0;
    reg_gpio_ien = 1;
    reg_gpio_oe = 1;

    configure_io();

    reg_uart_enable = 1;

    // Configure All LA probes as inputs to the cpu
	reg_la0_oenb = reg_la0_iena = 0x00000000;    // [31:0]
	reg_la1_oenb = reg_la1_iena = 0x00000000;    // [63:32]
	reg_la2_oenb = reg_la2_iena = 0x00000000;    // [95:64]
	reg_la3_oenb = reg_la3_iena = 0x00000000;    // [127:96]

	// write data to la output
    //	reg_la0_data = 0x00;
    //	reg_la1_data = 0x00;
    //	reg_la2_data = 0x00;
    //	reg_la3_data = 0x00;

    // read data from la input
    //	data0 = reg_la0_data;
    //	data1 = reg_la1_data;
    //	data2 = reg_la2_data;
    //	data3 = reg_la3_data;

    print("Hello World !!\n");

    stream_binary();

    // test_pattern();

// 	while (1) {

//         reg_gpio_out = 1; // OFF
//         reg_mprj_datal = 0x00000000;
//         reg_mprj_datah = 0x00000000;

// 		delay(800000);
// //		delay(8000000);

//         reg_gpio_out = 0;  // ON
//         reg_mprj_datah = 0x0000003f;
//         reg_mprj_datal = 0xffffffff;

// 		delay(800000);
// //		delay(8000000);

//     }
    // print()
    // int match = 1;
    // long int word = 0x7FF;
    // uint32_t pattern = 0;
    // while (match) {
    //     pattern = 0xA1B2C3D4 ^ word;
    //     MGMT_SRAM_BASE[word] = pattern;
    //     print("WORD "); print_hex(word, 8); print(": "); print("0x"); print_hex(MGMT_SRAM_BASE[word], 8); print("\n");
    //     // if(FLASH_BASE[word] != pattern) {
    //     //     match = 0;
    //     //     print("MISMATCH: expected 0x"); print_hex(pattern, 8); print(" but got 0x"); print_hex(MGMT_SRAM_BASE[word], 8); print("\n");
    //     // }
    //     word --;
    // }
    // print("SRAM size is "); print_dec(word - 1); print("words\n");

    // Print all contents of embedded binary
    // print_binary_contents();
}

