#include <defs.h>
#include <stub.h>

// SRAM address space
#define SRAM_BASE ((volatile uint32_t*)0x33000000)
#define MGMT_SRAM_BASE ((volatile uint32_t*)0x00000000)

#define WORD_NUM 128

#define RANDOM_TESTS 256
#define RANDOM_SEED 0x12345678

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

    reg_mprj_io_1 = GPIO_MODE_MGMT_STD_OUTPUT;
    reg_mprj_io_2 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;
    reg_mprj_io_3 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;
    reg_mprj_io_4 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;

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

// ========================================================
// Test state
// ========================================================

static uint32_t total_errors = 0;

// ========================================================
// Utility functions
// ========================================================

static void report_error(const char *test,
                         uint32_t index,
                         uint32_t expected,
                         uint32_t actual)
{
    total_errors++;

    print("ERROR ");
    print(test);
    print(" @ 0x");
    print_hex(index, 4);
    print(" Exp=0x");
    print_hex(expected, 8);
    print(" Got=0x");
    print_hex(actual, 8);
    print("\n");
}

// Simple deterministic 32-bit PRNG.
// xorshift32 is sufficient for memory stress testing.
static uint32_t prng_next(uint32_t *state)
{
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    *state = x;

    return x;
}

static uint32_t pattern_for_address(uint32_t address)
{
    // Deterministic address-dependent pattern.
    // This catches address/data aliasing better than a solid pattern.
    uint32_t x = address * 0x45D9F3B;
    x ^= x >> 16;
    x *= 0x45D9F3B;
    x ^= x >> 16;

    return x;
}

static void write_word (uint32_t addr, uint32_t data) {
    uint32_t actual;
    print("writing word...\n");
    for (int i = 0; i < 1000; i ++) {
        SRAM_BASE[addr] = data;
    }
    delay(100);
    actual = SRAM_BASE[addr];
    if (data != actual) {
        print("Expected 0x"); print_hex(data, 8); print(" ");
        print("Got 0x"); print_hex(actual, 8); print("\n");
    }
    print("Done.\n");
}

static void test_base_address(void) {
    uint32_t seed = RANDOM_SEED;
    uint32_t i;
    uint32_t actual;
    print("\n BASE ADDRESS TEST");
    for (int i = 0; i < RANDOM_TESTS; i++) {
        uint32_t exp = prng_next(&seed);
        for (int j = 0; j < 1000; j ++) {
            SRAM_BASE[0] = exp;
        }
        delay(100);
        actual = SRAM_BASE[0];
        if (exp != actual) {
            print("Expected 0x"); print_hex(exp, 8); print(" ");
            print("Got 0x"); print_hex(actual, 8); print("\n");
        }
    }
    print("  COMPLETE\n");
}

// ========================================================
// Test 0: read latency test
// ========================================================

static void test_read_latency(void)
{
    uint32_t i;

    print("\n[READ LATENCY TEST]\n");

    // Give every address a unique value.
    for (i = 0; i < WORD_NUM; i++) {
        SRAM_BASE[i] = pattern_for_address(i);
    }

    // Read each location twice.
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t first  = SRAM_BASE[i];
        uint32_t second = SRAM_BASE[i];

        uint32_t expected = pattern_for_address(i);

        if (first != expected) {
            print("FIRST  @ 0x");
            print_hex(i, 4);
            print(" Exp=0x");
            print_hex(expected, 8);
            print(" Got=0x");
            print_hex(first, 8);
            print("\n");
        }

        if (second != expected) {
            print("SECOND @ 0x");
            print_hex(i, 4);
            print(" Exp=0x");
            print_hex(expected, 8);
            print(" Got=0x");
            print_hex(second, 8);
            print("\n");
        }
    }
}

// ========================================================
// Test 1: Single-address write/read
// ========================================================

static void test_single_access(void)
{
    uint32_t i;
    uint32_t patterns[] = {
        0x00000000,
        0xFFFFFFFF,
        0x55555555,
        0xAAAAAAAA,
        0x12345678,
        0x87654321,
        0xDEADBEEF,
        0xCAFEBABE
    };

    print("\n[1] SINGLE ADDRESS WRITE/READ\n");

    for (i = 0; i < 8; i++) {

        uint32_t pattern = patterns[i];

        SRAM_BASE[0] = pattern;

        uint32_t read_val = SRAM_BASE[0];

        if (read_val != pattern) {
            report_error("single", 0, pattern, read_val);
        }
    }

    // Also test several nonzero addresses individually.
    for (i = 0; i < WORD_NUM; i += 37) {

        uint32_t pattern = pattern_for_address(i);

        SRAM_BASE[i] = pattern;

        uint32_t read_val = SRAM_BASE[i];

        if (read_val != pattern) {
            report_error("single", i, pattern, read_val);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 2: Address uniqueness
//
// Write each address with a unique value, then read it back.
// This catches address aliasing.
// ========================================================

static void test_address_uniqueness(void)
{
    uint32_t i;

    print("\n[2] ADDRESS UNIQUENESS\n");

    for (i = 0; i < WORD_NUM; i++) {
        SRAM_BASE[i] = pattern_for_address(i);
    }

    for (i = 0; i < WORD_NUM; i++) {

        uint32_t expected = pattern_for_address(i);
        uint32_t actual = SRAM_BASE[i];

        if (actual != expected) {
            report_error("address", i, expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 3: Walking-1 data test
//
// Each bit is independently exercised.
// ========================================================

static void test_walking_ones(void)
{
    uint32_t bit;
    uint32_t i;

    print("\n[3] WALKING-1 DATA TEST\n");

    for (bit = 0; bit < 32; bit++) {

        uint32_t pattern = 1u << bit;

        for (i = 0; i < WORD_NUM; i++) {
            SRAM_BASE[i] = pattern;
        }

        for (i = 0; i < WORD_NUM; i++) {

            uint32_t actual = SRAM_BASE[i];

            if (actual != pattern) {
                report_error("walk1", i, pattern, actual);
            }
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 4: Walking-0 data test
// ========================================================

static void test_walking_zeros(void)
{
    uint32_t bit;
    uint32_t i;

    print("\n[4] WALKING-0 DATA TEST\n");

    for (bit = 0; bit < 32; bit++) {

        uint32_t pattern = ~(1u << bit);

        for (i = 0; i < WORD_NUM; i++) {
            SRAM_BASE[i] = pattern;
        }

        for (i = 0; i < WORD_NUM; i++) {

            uint32_t actual = SRAM_BASE[i];

            if (actual != pattern) {
                report_error("walk0", i, pattern, actual);
            }
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 5: Solid patterns
// ========================================================

static void test_solid_patterns(void)
{
    uint32_t patterns[] = {
        0x00000000,
        0xFFFFFFFF,
        0x55555555,
        0xAAAAAAAA
    };

    uint32_t p;
    uint32_t i;

    print("\n[5] SOLID/CHECKERBOARD PATTERNS\n");

    for (p = 0; p < 4; p++) {

        uint32_t pattern = patterns[p];

        // Write entire memory.
        for (i = 0; i < WORD_NUM; i++) {
            SRAM_BASE[i] = pattern;
        }

        // Read entire memory.
        for (i = 0; i < WORD_NUM; i++) {

            uint32_t actual = SRAM_BASE[i];

            if (actual != pattern) {
                report_error("pattern", i, pattern, actual);
            }
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 6: Address-dependent data
//
// Every address receives a different deterministic pattern.
// ========================================================

static void test_address_patterns(void)
{
    uint32_t i;

    print("\n[6] ADDRESS-DEPENDENT PATTERN\n");

    for (i = 0; i < WORD_NUM; i++) {
        SRAM_BASE[i] = pattern_for_address(i);
    }

    for (i = 0; i < WORD_NUM; i++) {

        uint32_t expected = pattern_for_address(i);
        uint32_t actual = SRAM_BASE[i];

        if (actual != expected) {
            report_error("addrpat", i, expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 7: Sequential inverse patterns
//
// Alternates two values across adjacent addresses.
// ========================================================

static void test_alternating_addresses(void)
{
    uint32_t i;

    print("\n[7] ALTERNATING ADDRESS PATTERN\n");

    for (i = 0; i < WORD_NUM; i++) {

        if (i & 1)
            SRAM_BASE[i] = 0xAAAAAAAA;
        else
            SRAM_BASE[i] = 0x55555555;
    }

    for (i = 0; i < WORD_NUM; i++) {

        uint32_t expected;

        if (i & 1)
            expected = 0xAAAAAAAA;
        else
            expected = 0x55555555;

        uint32_t actual = SRAM_BASE[i];

        if (actual != expected) {
            report_error("alternate", i, expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 8: Forward/reverse validation
//
// Specifically stresses address traversal direction.
// ========================================================

static void test_forward_reverse(void)
{
    uint32_t i;

    print("\n[8] FORWARD/REVERSE VALIDATION\n");

    // Forward write.
    for (i = 0; i < WORD_NUM; i++) {
        SRAM_BASE[i] = pattern_for_address(i);
    }

    // Reverse read.
    for (i = WORD_NUM; i-- > 0;) {

        uint32_t expected = pattern_for_address(i);
        uint32_t actual = SRAM_BASE[i];

        if (actual != expected) {
            report_error("reverse-read", i, expected, actual);
        }
    }

    // Reverse write inverse.
    for (i = WORD_NUM; i-- > 0;) {
        SRAM_BASE[i] = ~pattern_for_address(i);
    }

    // Forward read.
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t expected = ~pattern_for_address(i);
        uint32_t actual = SRAM_BASE[i];

        if (actual != expected) {
            report_error("forward-read", i, expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 9: Random write / random read
//
// Uses a deterministic PRNG so failures are reproducible.
// ========================================================

static void test_random_access(void)
{
    uint32_t seed = RANDOM_SEED;
    uint32_t i;

    print("\n[9] RANDOM WRITE/READ TEST\n");

    // First create a deterministic random memory image.
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t data = prng_next(&seed);

        SRAM_BASE[i] = data;
    }

    // Reset PRNG to regenerate exactly the same data.
    seed = RANDOM_SEED;

    // Read in random address order.
    for (i = 0; i < RANDOM_TESTS; i++) {

        uint32_t address = prng_next(&seed) % WORD_NUM;

        // Generate expected value independently from address.
        // Use a second deterministic sequence.
        uint32_t expected_seed = RANDOM_SEED;
        uint32_t j;

        for (j = 0; j <= address; j++) {
            expected_seed = prng_next(&expected_seed);
        }

        uint32_t expected = expected_seed;
        uint32_t actual = SRAM_BASE[address];

        if (actual != expected) {
            report_error("random-read", address, expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 10: Random read/write stress
//
// Randomly selects an address and performs either a read
// or a write. Expected contents are tracked in software.
// ========================================================

static void test_random_stress(void)
{
    uint32_t expected_mem[WORD_NUM];

    uint32_t seed = RANDOM_SEED ^ 0xA5A5A5A5;
    uint32_t i;

    print("\n[10] RANDOM READ/WRITE STRESS\n");

    // Initialize software model and SRAM.
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t value = prng_next(&seed);

        expected_mem[i] = value;
        SRAM_BASE[i] = value;
    }

    // Random operations.
    for (i = 0; i < RANDOM_TESTS; i++) {

        uint32_t op = prng_next(&seed);
        uint32_t address = prng_next(&seed) % WORD_NUM;

        if (op & 1) {

            // WRITE
            uint32_t data = prng_next(&seed);

            SRAM_BASE[address] = data;
            expected_mem[address] = data;

        } else {

            // READ
            uint32_t actual = SRAM_BASE[address];
            uint32_t expected = expected_mem[address];

            if (actual != expected) {
                report_error("random-rw", address,
                             expected, actual);
            }
        }
    }

    // Final complete validation.
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t actual = SRAM_BASE[i];
        uint32_t expected = expected_mem[i];

        if (actual != expected) {
            report_error("random-final", i,
                         expected, actual);
        }
    }

    print("    COMPLETE\n");
}

// ========================================================
// Test 11: March X
// ========================================================

static void run_march_x_pass(uint32_t bg_pattern)
{
    uint32_t inv_pattern = ~bg_pattern;
    uint32_t i;

    print("\n[MARCH X] Pattern 0x");
    print_hex(bg_pattern, 8);
    print("\n");

    // ↑ w0
    for (i = 0; i < WORD_NUM; i++) {
        SRAM_BASE[i] = bg_pattern;
    }

    // ↑ r0,w1
    for (i = 0; i < WORD_NUM; i++) {

        uint32_t actual = SRAM_BASE[i];

        if (actual != bg_pattern) {
            report_error("march-r0", i,
                         bg_pattern, actual);
        }

        SRAM_BASE[i] = inv_pattern;
    }

    // ↓ r1,w0
    for (i = WORD_NUM; i-- > 0;) {

        uint32_t actual = SRAM_BASE[i];

        if (actual != inv_pattern) {
            report_error("march-r1", i,
                         inv_pattern, actual);
        }

        SRAM_BASE[i] = bg_pattern;
    }

    // ↓ r0
    for (i = WORD_NUM; i-- > 0;) {

        uint32_t actual = SRAM_BASE[i];

        if (actual != bg_pattern) {
            report_error("march-final", i,
                         bg_pattern, actual);
        }
    }

    print("    COMPLETE\n");
}

static void test_march_x(void)
{
    print("\n[11] MARCH X TEST\n");

    run_march_x_pass(0x00000000);
    run_march_x_pass(0x55555555);

    print("    COMPLETE\n");
}

// ========================================================
// Main SRAM test
// ========================================================

void run_sram_test(void)
{
    total_errors = 0;

    print("\n========================================\n");
    print("      COMPREHENSIVE SRAM TEST\n");
    print("========================================\n");

    print("Base address: 0x33000000\n");
    print("Words: ");
    print_dec(WORD_NUM);
    print("\n");

    test_read_latency();

    // Basic functionality first.
    test_single_access();

    // Address decoder / aliasing.
    test_address_uniqueness();

    // Individual data bits.
    test_walking_ones();
    test_walking_zeros();

    // Common memory test patterns.
    test_solid_patterns();

    // Address/data interaction.
    test_address_patterns();
    test_alternating_addresses();

    // Traversal direction.
    test_forward_reverse();

    // Random access.
    test_random_access();
    test_random_stress();

    // Formal March test.
    test_march_x();

    // ====================================================
    // Final result
    // ====================================================

    print("\n========================================\n");

    if (total_errors == 0) {
        print("SRAM TEST: PASS\n");
        print("All tests completed with 0 errors.\n");
    } else {
        print("SRAM TEST: FAIL\n");
        print("Total errors: ");
        print_dec(total_errors);
        print("\n");
    }

    print("========================================\n");
}

void main()
{
	int i, j, k;

    reg_spi_enable = 1;
    reg_wb_enable = 1;
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

    print("SRAM test firmware running...\n");
    // run_sram_test();
    uint32_t temp;
    // SRAM_BASE[0] = 0x00000000;
    // temp = SRAM_BASE[0];
    // print("0x"); print_hex(temp, 8); print("\n");
    // print("0x"); print_hex(SRAM_BASE[0], 8); print("\n");
    // SRAM_BASE[0] = 0xFFFFFFFF;
    // temp = SRAM_BASE[1];
    // print("0x"); print_hex(temp, 8); print("\n");
    // print("0x"); print_hex(SRAM_BASE[0], 8); print("\n");

    // test_base_address();

    //jump to management core SRAM base address (0x00000000)
    write_word(0, 0x00000067); //jalr x0 0(x0)
    print("0x"); print_hex(SRAM_BASE[0], 8); print("\n");
    MGMT_SRAM_BASE[0] = 0xDEADC0DE;
    temp = MGMT_SRAM_BASE[0];
    print("0x"); print_hex(temp, 8); print("\n");

    
    // print("0x"); print_hex(SRAM_BASE[0], 8); print("\n");
    // SRAM_BASE[0] = pattern_for_address(0);
    // SRAM_BASE[1] = pattern_for_address(1);
    // print("0x"); print_hex(SRAM_BASE[0], 8); print("\n");
    // print("0x"); print_hex(SRAM_BASE[1], 8); print("\n");
    // print("0x"); print_hex(pattern_for_address(0), 8); print("\n");
    // print("0x"); print_hex(pattern_for_address(1), 8); print("\n");

    // for (int i = 0; i < WORD_NUM; i ++) {
    //     SRAM_BASE[i] = pattern_for_address(i);
    // }
    // for (int i = 0; i < WORD_NUM; i ++) {
    //     print("0x"); print_hex(SRAM_BASE[i], 8); print("\n");
    //     print("0x"); print_hex(pattern_for_address(i), 8); print("\n");
    // }
    // // test_read_latency();
    print("SRAM test complete.\n");


}
