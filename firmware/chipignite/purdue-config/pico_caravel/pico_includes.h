#ifndef PICO_INCLUDES_H
#define PICO_INCLUDES_H

//includes
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/adc.h"
#include "pico/stdio.h"
#include <stdio.h>

// Pin Mappings
#define PIN_CARAVEL_PWR  7   // Enable line for 1V8/3V3 LDOs
#define PIN_RSTB         8   // Active Low Reset (~RESETB)
#define PIN_LED1         10  // Active Low LED 1
#define PIN_LED2         11  // Active Low LED 2

#define PIN_SPI_SI       12  // MOSI -> spi1 TX
#define PIN_SPI_CS       13  // CSn  -> spi1 CSn (Software controlled)
#define PIN_SPI_SCK      14  // SCK  -> spi1 SCK
#define PIN_SPI_SO       15  // MISO -> spi1 RX

#define PIN_UART_TXD     16  // RP2040 TX -> Caravel RX
#define PIN_UART_RXD     17  // RP2040 RX <- Caravel TX

#define PIN_ADC_3V3      26  // ADC Input (3.3V Rail Monitor)
#define PIN_ADC_1V8      27  // ADC Input (1.8V Rail Monitor)

#define PIN_CARAVEL_GP12      6
#define PIN_CARAVEL_GP13      9
#define PIN_CARAVEL_GP14      5
#define PIN_CARAVEL_GP15      4
#define PIN_CARAVEL_GP16      3
#define PIN_CARAVEL_GP17      2
#define PIN_CARAVEL_GP18      1
#define PIN_CARAVEL_GP19      0
#define PIN_CARAVEL_GP20_ADC3 29
#define PIN_CARAVEL_GP21_ADC2 28
#define PIN_CARAVEL_GP22      25
#define PIN_CARAVEL_GP23      24
#define PIN_CARAVEL_GP24      23
#define PIN_CARAVEL_GP25      22
#define PIN_CARAVEL_GP26      21
#define PIN_CARAVEL_GP27      20

// Command Opcodes over USB CDC
enum Cmd {
    CMD_SPI_EXCHANGE   = 0x01,
    CMD_SET_RSTB       = 0x02,
    CMD_SET_PWR_EN     = 0x03,
    CMD_READ_ADC_3V3   = 0x04,
    CMD_READ_ADC_1V8   = 0x05,
    CMD_SET_LED1       = 0x06,
    CMD_SET_LED2       = 0x07,
    CMD_POWER_CYCLE    = 0x08,
    CMD_RELEASE_PINS   = 0x09,
    CMD_CLAIM_PINS     = 0x0A,
    CMD_REBOOT_BOOTSEL = 0x0B,
};

static inline void gpio_init_out_val(uint pin, bool initial_val) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, initial_val);
}

// Measure ADC pin and return voltage in millivolts
uint16_t read_adc_mv(uint input_num) {
    adc_select_input(input_num);
    uint16_t raw = adc_read();
    // 12-bit ADC -> 3.3V reference: (raw * 3300) / 4095
    return (uint16_t)(((uint32_t)raw * 3300) / 4095);
}

//tri-state pico pins driving caravel I/Os
static void release_caravel_pins(void)
{
    // Stop SPI peripheral from driving its pins.
    spi_deinit(spi1);

    // Return SPI pins to GPIO control, then tri-state them.
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SPI_SI,  GPIO_FUNC_SIO);
    gpio_set_function(PIN_SPI_SO,  GPIO_FUNC_SIO);
    gpio_set_function(PIN_SPI_CS,  GPIO_FUNC_SIO);

    gpio_set_dir(PIN_SPI_SCK, GPIO_IN);
    gpio_set_dir(PIN_SPI_SI,  GPIO_IN);
    gpio_set_dir(PIN_SPI_SO,  GPIO_IN);
    gpio_set_dir(PIN_SPI_CS, GPIO_IN);

    // Release UART TX as well.
    gpio_set_function(PIN_UART_TXD, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_UART_TXD, GPIO_IN);

    // UART RX intentionally remains driven as an output.
    gpio_set_function(PIN_UART_RXD, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_UART_RXD, GPIO_OUT);
    gpio_put(PIN_UART_RXD, 0);
}

//claim caravel pins
static void claim_caravel_pins(void)
{
    // Reinitialize SPI1.
    spi_init(spi1, 1000 * 1000);

    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SI,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SO,  GPIO_FUNC_SPI);

    // CS is software controlled.
    gpio_init_out_val(PIN_SPI_CS, 1);
}

//main function declarations
int hkflash_main();
int teamXX_main();

#endif