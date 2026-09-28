//pin mappings + commands
#include "pico_includes.h"
#include "pico_team.h"

//ADD HELPERS BELOW


//ADD HELPERS ABOVE

int main() {
    caravel_init();
    //YOUR CODE BELOW

    // Configure ADC inputs.
    caravel_adc_init();
    
    // Configure Caravel SPI interface.
    caravel_spi_init(10 * 1000 * 1000);
    caravel_reset_deassert();

    while (true) {
        uint16_t sample;

        // Read raw 12-bit ADC value.
        sample = caravel_adc2_read();

        // Send the sample as two bytes.
        uint8_t tx[2] = {
            (uint8_t)(sample >> 8),
            (uint8_t)(sample & 0xff)
        };

        caravel_spi_select();
        caravel_spi_write(tx, 2);
        caravel_spi_deselect();

        sleep_us(10);
    }

    //YOUR CODE ABOVE
    while (true) {
        tight_loop_contents();
    }
    return 0;
}

/*
API list (for your convenience)

// Caravel Control
caravel_init(void);
caravel_reset_assert(void);
caravel_reset_deassert(void);
caravel_reset(int ms);
caravel_power_enable(void);
caravel_power_disable(void);
caravel_power_cycle(int ms);

// LEDs
caravel_led1(bool on);
caravel_led2(bool on);
caravel_led1_toggle(void);
caravel_led2_toggle(void);

// USB
caravel_usb_init(void);
caravel_usb_connected(void);
caravel_usb_write(const void *data, size_t length);
caravel_usb_write_byte(uint8_t data);
caravel_usb_flush(void);
caravel_usb_read_byte_timeout(uint32_t timeout_ms);
caravel_usb_read(void *data, size_t length, uint32_t timeout_ms);
caravel_usb_wait(void);
caravel_usb_print_hex(const char *name, uint32_t value);
caravel_usb_print_dec(const char *name, uint32_t value);

// GPIO
caravel_gpio_input(uint gpio);
caravel_gpio_output(uint gpio, bool value);
caravel_gpio_high_z(uint gpio);
caravel_gpio_pull_up(uint gpio);
caravel_gpio_pull_down(uint gpio);
caravel_gpio_set(uint gpio, bool value);
caravel_gpio_get(uint gpio);
caravel_gpio_toggle(uint gpio);
caravel_gpio_pulse(uint gpio, uint32_t duration_us);
caravel_gpio_clock(uint gpio, uint32_t frequency_hz, uint32_t cycles);

// Parallel GPIO Buses
caravel_bus_init_input(const caravel_bus_t *bus);
caravel_bus_init_output(const caravel_bus_t *bus, uint32_t initial_value);
caravel_bus_high_z(const caravel_bus_t *bus);
caravel_bus_read(const caravel_bus_t *bus);
caravel_bus_write(const caravel_bus_t *bus, uint32_t value);

// SPI
caravel_spi_init(uint32_t baudrate);
caravel_spi_init_default(void);
caravel_spi_select(void);
caravel_spi_deselect(void);
caravel_spi_transfer(uint8_t tx);
caravel_spi_transfer_blocking(const uint8_t *tx, uint8_t *rx, size_t length);
caravel_spi_write(const uint8_t *data, size_t length);
caravel_spi_read(uint8_t *data, size_t length);

// I2C
caravel_i2c_init(uint32_t baudrate);
caravel_i2c_init_default(void);
caravel_i2c_write(uint8_t address, const uint8_t *data, size_t length, bool no_stop);
caravel_i2c_read(uint8_t address, uint8_t *data, size_t length, bool no_stop);

// UART
caravel_uart_init(uint32_t baudrate);
caravel_uart_init_default(void);
caravel_uart_write_byte(uint8_t data);
caravel_uart_write(const uint8_t *data, size_t length);
caravel_uart_available(void);
caravel_uart_read_byte(void);

// ADC
caravel_adc_init(void);
caravel_adc_read_raw(uint channel);
caravel_adc2_read(void);
caravel_adc3_read(void);
caravel_adc_raw_to_mv(uint16_t raw);
caravel_adc2_read_mv(void);
caravel_adc3_read_mv(void);

// PWM
caravel_pwm_init(uint gpio, uint32_t frequency_hz, uint16_t duty_cycle);
caravel_pwm_set_duty(uint gpio, uint16_t duty_cycle);

// PIO
caravel_pio_claim(PIO pio);
caravel_pio_release(PIO pio, uint sm);

// DMA
caravel_dma_claim(void);
caravel_dma_release(uint channel);

// Timing
caravel_delay_us(uint32_t us);
caravel_delay_ms(uint32_t ms);
caravel_time_us(void);

*/