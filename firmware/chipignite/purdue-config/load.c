/*
 * SPDX-FileCopyrightText: 2020 Efabless Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * SPDX-License-Identifier: Apache-2.0
 */

// This include is relative to $CARAVEL_PATH (see Makefile)
#include <defs.h>
#include <stub.h>

// List of Wishbone Slave Addresses
// Sample Project                                        0x300TXXXX //T must match team number, XXXX can be anything divisible by 4
#define reg_sample_proj_EN (*(volatile uint32_t*)        0x30000000)
#define reg_sample_proj_PRESCALER (*(volatile uint32_t*) 0x30000004)
#define reg_sample_proj_IM (*(volatile uint32_t*)        0x3000FF00)
#define reg_sample_proj_MIS (*(volatile uint32_t*)       0x3000FF04)
#define reg_sample_proj_RIS (*(volatile uint32_t*)       0x3000FF08)
#define reg_sample_proj_IC (*(volatile uint32_t*)        0x3000FF0C)

// GPIO Control
#define reg_gpio_PIN_0TO7 (*(volatile uint32_t*)0x32000000)
#define reg_gpio_PIN_8TO15 (*(volatile uint32_t*)0x32000004)
#define reg_gpio_PIN_16TO23 (*(volatile uint32_t*)0x32000008)
#define reg_gpio_PIN_24TO31 (*(volatile uint32_t*)0x3200000C)
#define reg_gpio_PIN_32TO37 (*(volatile uint32_t*)0x32000010)

// LA Control
#define reg_la_sel (*(volatile uint32_t*)0x31000000)

/*
	Sample Team Project Test:
		- Configures all IO pins as outputs
		- Configures all IO and LA pins to be selected by sample project
		- Enables sample project design through WB
		- Enables design's output cycling through LA inputs
		- Checks GPIO outputs consistently
		- Stops output cycling through LA inputs
		- "Acknowledges" interrupt, and enables cycling again
*/

void main()
{

    reg_gpio_mode1 = 1;
    reg_gpio_mode0 = 0;
    reg_gpio_ien = 1;
    reg_gpio_oe = 1;

	/* Set up the housekeeping SPI to be connected internally so	*/
	/* that external pin changes don't affect it.			*/

	reg_spi_enable = 1;
    reg_wb_enable = 1;
	// reg_spimaster_cs = 0x10001;
	// reg_spimaster_control = 0x0801;

	// reg_spimaster_control = 0xa002;	// Enable, prescaler = 2,
                                        // connect to housekeeping SPI

    //enable UART
    reg_uart_enable = 1;

	// Connect the housekeeping SPI to the SPI master
	// so that the CSB line is not left floating.  This allows
	// all of the GPIO pins to be used for user functions.

	// Configure IO[0] and IO[37:5] to outputs
	reg_mprj_io_0 =  GPIO_MODE_USER_STD_OUT_MONITORED;
	// reg_mprj_io_5 = GPIO_MODE_USER_STD_OUTPUT;
    // reg_mprj_io_6 = GPIO_MODE_USER_STD_OUTPUT;
	reg_mprj_io_5 = GPIO_MODE_MGMT_STD_INPUT_NOPULL;
    reg_mprj_io_6 = GPIO_MODE_MGMT_STD_OUTPUT;

    reg_mprj_io_7 =  GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_8 =  GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_9 =  GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_10 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_11 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_12 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_13 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_14 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_15 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_16 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_17 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_18 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_19 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_20 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_21 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_22 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_23 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_24 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_25 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_26 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_27 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_28 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_29 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_30 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_31 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_32 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_33 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_34 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_35 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_36 = GPIO_MODE_USER_STD_OUT_MONITORED;
    reg_mprj_io_37 = GPIO_MODE_USER_STD_OUT_MONITORED;

	// Now, apply configuration
	reg_mprj_xfer = 1;
	while (reg_mprj_xfer == 1);

	// Configure All LA probes as inputs to the cpu 
	reg_la0_oenb = reg_la0_iena = 0x00000000;    // [31:0]
	reg_la1_oenb = reg_la1_iena = 0x00000000;    // [63:32]
	reg_la2_oenb = reg_la2_iena = 0x00000000;    // [95:64]
	reg_la3_oenb = reg_la3_iena = 0x00000000;    // [127:96]

	// Configure GPIOs outputs to be selected by sample project
	reg_gpio_PIN_0TO7 = 0x00000000;//team 1 would be all 1s, team 2 would be all 2s
	reg_gpio_PIN_8TO15 = 0x00000000;
	reg_gpio_PIN_16TO23 = 0x00000000;
	reg_gpio_PIN_24TO31 = 0x00000000;
	reg_gpio_PIN_32TO37 = 0x000000;

	// Configure LA output to be selected by sample project
	reg_la_sel = 0x0;

	// Enable the sample project design
	reg_sample_proj_EN = 0x1;

	// Enable interrupt mask
	reg_sample_proj_IM = 0x1;

	// Set "prescaler" value to 1
	reg_sample_proj_PRESCALER = 0x50;

	// Configure LA[0] LA[1] as outputs from the cpu
	reg_la0_oenb = reg_la0_iena = 0x00000003;

    reg_la0_data = 0x1;  // Set "enable" high

    uint64_t gpio_data;
    uint64_t gpio_data_last;

	// Normal design operation
	while (1) {
        gpio_data_last = gpio_data;
        gpio_data = reg_mprj_datal & ~(0x3 << 5);
        gpio_data |= reg_mprj_datah << 32;
        if (gpio_data != gpio_data_last) {
            print("GPIO: 0x"); print_hex(reg_mprj_datah, 8); print_hex(reg_mprj_datal, 8); print("\n");
            if (reg_mprj_datal & 0x1) {
                reg_gpio_out = 0x0;
            } else {
                reg_gpio_out = 0x1;
            }
        }
	}

}
