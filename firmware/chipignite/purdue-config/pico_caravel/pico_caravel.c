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

int main() {
    stdio_init_all(); // Init USB CDC Serial

    // Initialize Power & Reset Lines
    gpio_init_out_val(PIN_CARAVEL_PWR, 1); // Turn power on by default
    gpio_init_out_val(PIN_RSTB, 1);        // Deassert Reset (~RESETB High)

    // Initialize LEDs (Active Low)
    gpio_init_out_val(PIN_LED1, 1);
    gpio_init_out_val(PIN_LED2, 1);

    // Initialize spi1 at 1 MHz, CPOL=0, CPHA=0
    spi_init(spi1, 1000 * 1000);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SI,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SO,  GPIO_FUNC_SPI);
    gpio_init_out_val(PIN_SPI_CS, 1); // Manual CS for strict packet size control

    // Initialize UART0 at 115200 baud
    uart_init(uart0, 115200);
    gpio_set_function(PIN_UART_TXD, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RXD, GPIO_FUNC_UART);

    // Initialize ADC
    adc_init();
    adc_gpio_init(PIN_ADC_3V3);
    adc_gpio_init(PIN_ADC_1V8);

    while (1) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) continue;

        switch ((uint8_t)c) {
            case CMD_SPI_EXCHANGE: {
                // Protocol: [CMD] [LEN_HI] [LEN_LO] [DATA...]
                uint16_t len = ((uint8_t)getchar() << 8) | (uint8_t)getchar();
                uint8_t buf[1024];

                for (uint16_t i = 0; i < len; i++) {
                    buf[i] = (uint8_t)getchar();
                }

                gpio_put(PIN_SPI_CS, 0); // Assert CS
                spi_write_read_blocking(spi1, buf, buf, len);
                gpio_put(PIN_SPI_CS, 1); // Deassert CS

                for (uint16_t i = 0; i < len; i++) {
                    putchar_raw(buf[i]);
                }
                break;
            }

            case CMD_SET_RSTB:
                gpio_put(PIN_RSTB, getchar() & 1);
                break;

            case CMD_SET_PWR_EN:
                gpio_put(PIN_CARAVEL_PWR, getchar() & 1);
                break;

            case CMD_READ_ADC_3V3: {
                uint16_t mv = read_adc_mv(0); // GP26 is ADC0
                putchar_raw((mv >> 8) & 0xFF);
                putchar_raw(mv & 0xFF);
                break;
            }

            case CMD_READ_ADC_1V8: {
                uint16_t mv = read_adc_mv(1); // GP27 is ADC1
                putchar_raw((mv >> 8) & 0xFF);
                putchar_raw(mv & 0xFF);
                break;
            }

            case CMD_SET_LED1:
                gpio_put(PIN_LED1, !(getchar() & 1)); // Active Low
                break;

            case CMD_SET_LED2:
                gpio_put(PIN_LED2, !(getchar() & 1)); // Active Low
                break;

            case CMD_POWER_CYCLE: {
                // Safely power cycle: cut LDO enable and wait for rails to drop below 100mV
                gpio_put(PIN_CARAVEL_PWR, 0);
                while (read_adc_mv(0) > 100 || read_adc_mv(1) > 100) {
                    sleep_ms(10);
                }
                sleep_ms(50); // Extra discharge settling margin
                gpio_put(PIN_CARAVEL_PWR, 1);
                putchar_raw(0x01); // ACK when complete
                break;
            }

            case CMD_RELEASE_PINS:
                release_caravel_pins();
                putchar_raw(0x1); //Ack
                break;

            case CMD_CLAIM_PINS:
                claim_caravel_pins();
                putchar_raw(0x1); //Ack
                break;
                
            default:
                break;
        }
    }
}