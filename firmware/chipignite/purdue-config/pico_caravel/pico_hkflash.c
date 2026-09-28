//pin mappings + commands
#include "pico_includes.h"

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

    // Initialize UART0 at 9600 baud
    uart_init(uart0, 9600);
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
            
            case CMD_REBOOT_BOOTSEL:
                //never returns
                // reset_usb_boot(0, 0); //TODO
                break;

            default:
                break;
        }
    }
    return 1;
}