#ifndef PICO_TEAM_H
#define PICO_TEAM_H

/*
 * team_caravel.h
 *
 * Team-facing utilities for RP2040 Caravel test firmware.
 *
 * This header is intentionally independent of the Caravel flash/programming
 * bridge. It provides simple wrappers around common RP2040 peripherals so
 * teams can interact with their Caravel user project.
 *
 * IMPORTANT:
 *   caravel_init() does NOT claim/configure SPI, I2C, UART, ADC, or
 *   Caravel GPIO pins. Teams must explicitly initialize the interfaces
 *   they intend to use.
 *
 * Physical RP2040 pin assignments are defined by pico_includes.h.
 */

#include "pico_includes.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * ============================================================================
 * Clock Domain Considerations
 * ============================================================================
 *
 * The RP2040 and Caravel do NOT need to share a clock.
 *
 * The following interfaces are designed to operate between independent
 * clock domains:
 *
 *     GPIO   - asynchronous digital signals; synchronize inside Caravel
 *     SPI    - SPI clock provides the interface timing
 *     I2C    - SCL provides the interface timing
 *     UART   - asynchronous serial interface
 *     ADC    - RP2040 samples independently
 *     USB    - handled internally by the RP2040 USB peripheral
 *     PIO    - can use an external/interface clock for sampling
 *
 * For multi-bit asynchronous GPIO buses, do not independently synchronize
 * each bit and assume the resulting word is coherent. Use a handshake,
 * strobe, source-synchronous clock, or asynchronous FIFO.
 *
 * For high-speed parallel interfaces, PIO + DMA is recommended.
 */

 // ============================================================================
// Caravel Control
// ============================================================================

/* Initialize Caravel power, reset, and LEDs. Does not claim interface pins. */
caravel_init(void);

/* Assert Caravel reset. */
caravel_reset_assert(void);

/* Release Caravel reset. */
caravel_reset_deassert(void);

/* Assert reset for the specified number of milliseconds, then release. */
caravel_reset(int ms);

/* Enable Caravel power. */
caravel_power_enable(void);

/* Disable Caravel power. */
caravel_power_disable(void);

/* Disable Caravel power for the specified number of milliseconds, then enable. */
caravel_power_cycle(int ms);


// ============================================================================
// LEDs
// ============================================================================

/* Turn LED1 on or off. */
caravel_led1(bool on);

/* Turn LED2 on or off. */
caravel_led2(bool on);

/* Toggle LED1. */
caravel_led1_toggle(void);

/* Toggle LED2. */
caravel_led2_toggle(void);


// ============================================================================
// USB
// ============================================================================

/* Initialize USB CDC / stdio. */
caravel_usb_init(void);

/* Return true if the host is connected over USB. */
caravel_usb_connected(void);

/* Write a buffer of raw bytes to USB. */
caravel_usb_write(const void *data, size_t length);

/* Write one byte to USB. */
caravel_usb_write_byte(uint8_t data);

/* Flush pending USB output. */
caravel_usb_flush(void);

/* Read one byte from USB, waiting up to timeout_ms. Returns -1 on timeout. */
caravel_usb_read_byte_timeout(uint32_t timeout_ms);

/* Read up to length bytes from USB. Returns the number of bytes received. */
caravel_usb_read(void *data, size_t length, uint32_t timeout_ms);

/* Wait until a USB host is connected. */
caravel_usb_wait(void);

/* Print a 32-bit value in hexadecimal over USB. */
caravel_usb_print_hex(const char *name, uint32_t value);

/* Print a 32-bit value in decimal over USB. */
caravel_usb_print_dec(const char *name, uint32_t value);


// ============================================================================
// GPIO
// ============================================================================

/* Configure a Caravel-connected pin as a digital input. */
caravel_gpio_input(uint gpio);

/* Configure a Caravel-connected pin as a digital output. */
caravel_gpio_output(uint gpio, bool value);

/* Configure a pin as high impedance with pulls disabled. */
caravel_gpio_high_z(uint gpio);

/* Configure an input with an internal pull-up. */
caravel_gpio_pull_up(uint gpio);

/* Configure an input with an internal pull-down. */
caravel_gpio_pull_down(uint gpio);

/* Set the value of an output pin. */
caravel_gpio_set(uint gpio, bool value);

/* Read the value of an input pin. */
caravel_gpio_get(uint gpio);

/* Toggle the value of an output pin. */
caravel_gpio_toggle(uint gpio);

/* Drive a pin high for the specified number of microseconds, then low. */
caravel_gpio_pulse(uint gpio, uint32_t duration_us);

/* Generate a simple square-wave clock for the specified number of cycles. */
caravel_gpio_clock(
    uint gpio,
    uint32_t frequency_hz,
    uint32_t cycles);


// ============================================================================
// Parallel GPIO Buses
// ============================================================================

/*
 * Define a configurable parallel bus.
 *
 * Example:
 *
 * const uint8_t pins[] = {
 *     PIN_CARAVEL_GP20,
 *     PIN_CARAVEL_GP21,
 *     PIN_CARAVEL_GP22,
 *     PIN_CARAVEL_GP23
 * };
 *
 * const caravel_bus_t bus = {
 *     .pins = pins,
 *     .width = 4
 * };
 */
typedef struct {
    const uint8_t *pins;
    uint8_t width;
} caravel_bus_t;

/* Configure all bus pins as inputs. */
caravel_bus_init_input(const caravel_bus_t *bus);

/* Configure all bus pins as outputs and set their initial value. */
caravel_bus_init_output(
    const caravel_bus_t *bus,
    uint32_t initial_value);

/* Put all bus pins into high impedance. */
caravel_bus_high_z(const caravel_bus_t *bus);

/* Read the entire parallel bus. */
caravel_bus_read(const caravel_bus_t *bus);

/* Write a value to the entire parallel bus. */
caravel_bus_write(
    const caravel_bus_t *bus,
    uint32_t value);


// ============================================================================
// SPI
// ============================================================================

/*
 * Initialize the Caravel SPI interface.
 *
 * Claims the configurable SPI SCK, SI, SO, and CS pins.
 */
caravel_spi_init(uint32_t baudrate);

/* Initialize SPI using CARAVEL_SPI_BAUD. */
caravel_spi_init_default(void);

/* Assert SPI chip select. */
caravel_spi_select(void);

/* Deassert SPI chip select. */
caravel_spi_deselect(void);

/* Transfer one byte and return the received byte. */
caravel_spi_transfer(uint8_t tx);

/* Perform a full-duplex SPI buffer transfer. */
caravel_spi_transfer_blocking(
    const uint8_t *tx,
    uint8_t *rx,
    size_t length);

/* Write a buffer over SPI. */
caravel_spi_write(
    const uint8_t *data,
    size_t length);

/* Read a buffer over SPI while transmitting zeroes. */
caravel_spi_read(
    uint8_t *data,
    size_t length);


// ============================================================================
// I2C
// ============================================================================

/* Initialize the configurable I2C interface. */
caravel_i2c_init(uint32_t baudrate);

/* Initialize I2C using CARAVEL_I2C_BAUD. */
caravel_i2c_init_default(void);

/*
 * Write bytes to an I2C device.
 *
 * Returns the number of bytes written, or a negative error code.
 */
caravel_i2c_write(
    uint8_t address,
    const uint8_t *data,
    size_t length,
    bool no_stop);

/*
 * Read bytes from an I2C device.
 *
 * Returns the number of bytes read, or a negative error code.
 */
caravel_i2c_read(
    uint8_t address,
    uint8_t *data,
    size_t length,
    bool no_stop);


// ============================================================================
// UART
// ============================================================================

/* Initialize the Caravel UART. */
caravel_uart_init(uint32_t baudrate);

/* Initialize UART using CARAVEL_UART_BAUD. */
caravel_uart_init_default(void);

/* Send one byte over the Caravel UART. */
caravel_uart_write_byte(uint8_t data);

/* Send a buffer over the Caravel UART. */
caravel_uart_write(
    const uint8_t *data,
    size_t length);

/* Return true if a UART byte is available. */
caravel_uart_available(void);

/* Read one UART byte. Returns -1 if no byte is available. */
caravel_uart_read_byte(void);


// ============================================================================
// ADC
// ============================================================================

/*
 * Initialize Caravel ADC inputs.
 *
 * ADC2 = RP2040 GPIO28 = Caravel GP21
 * ADC3 = RP2040 GPIO29 = Caravel GP20
 */
caravel_adc_init(void);

/* Read an arbitrary RP2040 ADC channel and return the raw 12-bit value. */
caravel_adc_read_raw(uint channel);

/* Read Caravel GP21 / RP2040 ADC2. */
caravel_adc2_read(void);

/* Read Caravel GP20 / RP2040 ADC3. */
caravel_adc3_read(void);

/* Convert a raw 12-bit ADC value to millivolts. */
caravel_adc_raw_to_mv(uint16_t raw);

/* Read ADC2 and return the result in millivolts. */
caravel_adc2_read_mv(void);

/* Read ADC3 and return the result in millivolts. */
caravel_adc3_read_mv(void);


// ============================================================================
// PWM
// ============================================================================

/*
 * Configure a Caravel-connected GPIO as PWM.
 *
 * duty_cycle is specified in thousandths:
 *     0    = 0%
 *     500  = 50%
 *     1000 = 100%
 *
 * Returns the PWM slice number.
 */
caravel_pwm_init(
    uint gpio,
    uint32_t frequency_hz,
    uint16_t duty_cycle);

/* Change the PWM duty cycle on an already-configured pin. */
caravel_pwm_set_duty(
    uint gpio,
    uint16_t duty_cycle);


// ============================================================================
// PIO
// ============================================================================

/* Claim an unused PIO state machine. Returns -1 if none are available. */
caravel_pio_claim(PIO pio);

/* Release a previously claimed PIO state machine. */
caravel_pio_release(PIO pio, uint sm);


// ============================================================================
// DMA
// ============================================================================

/* Claim an unused DMA channel. Returns -1 if none are available. */
caravel_dma_claim(void);

/* Release a previously claimed DMA channel. */
caravel_dma_release(uint channel);


// ============================================================================
// Timing
// ============================================================================

/* Delay for the specified number of microseconds. */
caravel_delay_us(uint32_t us);

/* Delay for the specified number of milliseconds. */
caravel_delay_ms(uint32_t ms);

/* Return the number of microseconds since RP2040 startup. */
caravel_time_us(void);





/* ============================================================================
 * Configuration
 * ========================================================================== */

/*
 * SPI
 *
 * The same physical SPI pins may instead be used as ordinary GPIOs.
 * Calling caravel_spi_init() claims them for SPI.
 */
#ifndef CARAVEL_SPI
#define CARAVEL_SPI             spi1
#endif
#ifndef CARAVEL_SPI_BAUD
#define CARAVEL_SPI_BAUD        1000000u
#endif
#ifndef CARAVEL_SPI_SCK
#define CARAVEL_SPI_SCK         PIN_SPI_SCK
#endif
#ifndef CARAVEL_SPI_TX
#define CARAVEL_SPI_TX          PIN_SPI_SI
#endif
#ifndef CARAVEL_SPI_RX
#define CARAVEL_SPI_RX          PIN_SPI_SO
#endif
#ifndef CARAVEL_SPI_CS
#define CARAVEL_SPI_CS          PIN_SPI_CS
#endif

/*
 * I2C
 *
 * These are RP2040 GPIO numbers, not Caravel GP numbers.
 * Change them if the team's PCB/interface requires different pins.
 */
#ifndef CARAVEL_I2C
#define CARAVEL_I2C             i2c0
#endif
#ifndef CARAVEL_I2C_SDA
#define CARAVEL_I2C_SDA         18
#endif
#ifndef CARAVEL_I2C_SCL
#define CARAVEL_I2C_SCL         19
#endif
#ifndef CARAVEL_I2C_BAUD
#define CARAVEL_I2C_BAUD        400000u
#endif

/*
 * UART
 *
 * These pins are the dedicated Caravel UART connection.
 */
#ifndef CARAVEL_UART
#define CARAVEL_UART            uart0
#endif
#ifndef CARAVEL_UART_TX
#define CARAVEL_UART_TX         PIN_UART_TXD
#endif
#ifndef CARAVEL_UART_RX
#define CARAVEL_UART_RX         PIN_UART_RXD
#endif
#ifndef CARAVEL_UART_BAUD
#define CARAVEL_UART_BAUD       9600u
#endif

/*
 * ADC
 *
 * Caravel GP20 -> RP2040 GPIO29 -> ADC3
 * Caravel GP21 -> RP2040 GPIO28 -> ADC2
 */
#ifndef CARAVEL_ADC2_GPIO
#define CARAVEL_ADC2_GPIO       PIN_CARAVEL_GP21_ADC2
#endif
#ifndef CARAVEL_ADC3_GPIO
#define CARAVEL_ADC3_GPIO       PIN_CARAVEL_GP20_ADC3
#endif
#ifndef CARAVEL_ADC2_CHANNEL
#define CARAVEL_ADC2_CHANNEL    2u
#endif
#ifndef CARAVEL_ADC3_CHANNEL
#define CARAVEL_ADC3_CHANNEL    3u
#endif
#ifndef CARAVEL_ADC_VREF_MV
#define CARAVEL_ADC_VREF_MV     3300u
#endif

/* ============================================================================
 * Caravel power / reset / LEDs
 * ========================================================================== */

/*
 * Initialize the shared Caravel control signals.
 *
 * This function intentionally does NOT initialize any communication interface
 * or Caravel user GPIO.
 */
static inline void caravel_init(void) {
    gpio_init(PIN_CARAVEL_PWR);
    gpio_set_dir(PIN_CARAVEL_PWR, GPIO_OUT);
    gpio_put(PIN_CARAVEL_PWR, 1);
    gpio_init(PIN_RSTB);
    gpio_set_dir(PIN_RSTB, GPIO_OUT);
    gpio_put(PIN_RSTB, 0);
    gpio_init(PIN_LED1);
    gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_init(PIN_LED2);
    gpio_set_dir(PIN_LED2, GPIO_OUT);
    /* LEDs are active low. */
    gpio_put(PIN_LED1, 1);
    gpio_put(PIN_LED2, 1);
}

/* Assert Caravel reset. */
static inline void caravel_reset_assert(void) { gpio_put(PIN_RSTB, 0); }

/* Release Caravel reset. */
static inline void caravel_reset_deassert(void) { gpio_put(PIN_RSTB, 1); }

/* Assert reset, wait, then release reset. */
static inline void caravel_reset(uint32_t duration_us) {
    caravel_reset_assert();
    sleep_us(duration_us);
    caravel_reset_deassert();
}

typedef struct {
    uint32_t dir;          /* Direction register (1 = Output, 0 = Input) */
    uint32_t pad_ctrl[30]; /* Individual pad control registers (pulls, etc.) */
    bool is_saved;         /* Protection flag to avoid double-overwriting */
} caravel_power_gpio_state_t;

static caravel_power_gpio_state_t g_caravel_gpio_saved_state = {0};

/* Save state of all pins (except power, reset, LEDs) and set them to High-Z */
static inline void caravel_power_tristate_all(void) {
    if (g_caravel_gpio_saved_state.is_saved) return;

    /* 1. Save direction state */
    g_caravel_gpio_saved_state.dir = gpio_get_dir_all();

    /* 2. Save individual pad configurations and tri-state non-exempt pins */
    for (uint gpio = 0; gpio < 30; gpio++) {
        /* Skip control pins */
        if (gpio == PIN_CARAVEL_PWR || gpio == PIN_RSTB || 
            gpio == PIN_LED1 || gpio == PIN_LED2) {
            continue;
        }

        /* Save pad control register (contains pull-up/pull-down/drive strength settings) */
        g_caravel_gpio_saved_state.pad_ctrl[gpio] = pads_bank0_hw->io[gpio];

        /* Put pin into High-Z (input mode with pulls disabled) */
        gpio_set_dir(gpio, GPIO_IN);
        gpio_disable_pulls(gpio);
    }

    g_caravel_gpio_saved_state.is_saved = true;
}

/* Restore all pins to their saved direction and pad configurations */
static inline void caravel_power_restore_all(void) {
    if (!g_caravel_gpio_saved_state.is_saved) return;

    for (uint gpio = 0; gpio < 30; gpio++) {
        if (gpio == PIN_CARAVEL_PWR || gpio == PIN_RSTB || 
            gpio == PIN_LED1 || gpio == PIN_LED2) {
            continue;
        }

        /* Restore pad controls (pull-ups/pull-downs) */
        pads_bank0_hw->io[gpio] = g_caravel_gpio_saved_state.pad_ctrl[gpio];

        /* Restore direction */
        bool is_output = (g_caravel_gpio_saved_state.dir >> gpio) & 1u;
        gpio_set_dir(gpio, is_output ? GPIO_OUT : GPIO_IN);
    }

    g_caravel_gpio_saved_state.is_saved = false;
}

/* Disable Caravel power. */
static inline void caravel_power_disable(void) {
    caravel_power_tristate_all();
    gpio_put(PIN_CARAVEL_PWR, 0);
}

/* Enable Caravel power. */
static inline void caravel_power_enable(void) {
    gpio_put(PIN_CARAVEL_PWR, 1);
    caravel_power_restore_all();
}

/* Power-cycle Caravel. */
static inline void caravel_power_cycle(uint32_t off_time_ms) {
    caravel_power_disable();
    sleep_ms(off_time_ms);
    caravel_power_enable();
}

/* Set LED1 state. */
static inline void caravel_led1(bool on) { gpio_put(PIN_LED1, !on); }

/* Set LED2 state. */
static inline void caravel_led2(bool on) { gpio_put(PIN_LED2, !on); }

/* Toggle LED1. */
static inline void caravel_led1_toggle(void) { gpio_put(PIN_LED1, !gpio_get(PIN_LED1)); }

/* Toggle LED2. */
static inline void caravel_led2_toggle(void) { gpio_put(PIN_LED2, !gpio_get(PIN_LED2)); }

/* ============================================================================
 * USB
 * ========================================================================== */

/*
 * Initialize USB CDC / stdio.
 *
 * The CMake target should have:
 *
 *     pico_enable_stdio_usb(pico_teamXX 1)
 *     pico_enable_stdio_uart(pico_teamXX 0)
 */
static inline void caravel_usb_init(void) { stdio_init_all(); }

/* Return true when USB CDC is connected. */
static inline bool caravel_usb_connected(void) { return stdio_usb_connected(); }

/* Write raw bytes to USB CDC. */
static inline void caravel_usb_write(const void *data, size_t length) { fwrite(data, 1, length, stdout); }

/* Write one byte to USB CDC. */
static inline void caravel_usb_write_byte(uint8_t data) { putchar_raw(data); }

/* Flush USB output. */
static inline void caravel_usb_flush(void) { fflush(stdout); }

/*
 * Check whether a byte is available from USB CDC.
 *
 * Returns:
 *   true  = byte available
 *   false = no byte available
 */
static inline bool caravel_usb_available(void) {
    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT) return false;
    /*
     * getchar_timeout_us() consumes the byte. This helper is therefore only
     * useful as a simple polling primitive when a byte itself is not needed.
     *
     * For packet-oriented protocols, use caravel_usb_read_byte_timeout()
     * instead.
     */
    return true;
}

/*
 * Read one byte with a timeout.
 *
 * Returns:
 *   0..255 = received byte
 *   -1     = timeout
 */
static inline int caravel_usb_read_byte_timeout(uint32_t timeout_ms) {
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (true) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) return c;
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) return -1;
        tight_loop_contents();
    }
}

/*
 * Read up to length bytes from USB.
 *
 * Returns the number of bytes received.
 *
 * This is intentionally a simple polling interface rather than a background
 * USB packet buffer.
 */
static inline size_t caravel_usb_read(void *data, size_t length, uint32_t timeout_ms) {
    uint8_t *dst = (uint8_t *)data;
    size_t count = 0;
    while (count < length) {
        int c = caravel_usb_read_byte_timeout(timeout_ms);
        if (c < 0) break;
        dst[count++] = (uint8_t)c;
    }
    return count;
}

/* ============================================================================
 * Generic Caravel GPIO
 * ========================================================================== */

/* Configure a Caravel-connected pin as an input. */
static inline void caravel_gpio_input(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
}

/* Configure a Caravel-connected pin as an output. */
static inline void caravel_gpio_output(uint gpio, bool value) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, value);
}

/* Configure a pin as high impedance. */
static inline void caravel_gpio_high_z(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_disable_pulls(gpio);
}

/* Enable a pull-up input. */
static inline void caravel_gpio_pull_up(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

/* Enable a pull-down input. */
static inline void caravel_gpio_pull_down(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_down(gpio);
}

/* Set an already-configured GPIO. */
static inline void caravel_gpio_set(uint gpio, bool value) { gpio_put(gpio, value); }

/* Read an already-configured GPIO. */
static inline bool caravel_gpio_get(uint gpio) { return gpio_get(gpio); }

/* Toggle an already-configured GPIO. */
static inline void caravel_gpio_toggle(uint gpio) { gpio_xor_mask(1u << gpio); }

/* ============================================================================
 * Parallel GPIO buses
 * ========================================================================== */

typedef struct {
    const uint8_t *pins;
    uint8_t width;
} caravel_bus_t;

/* Configure every bus pin as an input. */
static inline void caravel_bus_init_input(const caravel_bus_t *bus) {
    for (uint i = 0; i < bus->width; i++) caravel_gpio_input(bus->pins[i]);
}

/* Configure every bus pin as an output. */
static inline void caravel_bus_init_output(const caravel_bus_t *bus, uint32_t initial_value) {
    for (uint i = 0; i < bus->width; i++) {
        caravel_gpio_output(bus->pins[i], (initial_value >> i) & 1u);
    }
}

/* Put every bus pin into high impedance. */
static inline void caravel_bus_high_z(const caravel_bus_t *bus) {
    for (uint i = 0; i < bus->width; i++) caravel_gpio_high_z(bus->pins[i]);
}

/* Read a parallel bus. */
static inline uint32_t caravel_bus_read(const caravel_bus_t *bus) {
    uint32_t value = 0;
    for (uint i = 0; i < bus->width; i++) {
        if (gpio_get(bus->pins[i])) value |= (1u << i);
    }
    return value;
}

/* Write a parallel bus. */
static inline void caravel_bus_write(const caravel_bus_t *bus, uint32_t value) {
    for (uint i = 0; i < bus->width; i++) gpio_put(bus->pins[i], (value >> i) & 1u);
}

/* ============================================================================
 * SPI
 * ========================================================================== */

/*
 * Initialize the Caravel SPI interface.
 *
 * This claims:
 *   PIN_SPI_SCK
 *   PIN_SPI_SI
 *   PIN_SPI_SO
 *   PIN_SPI_CS
 */
static inline void caravel_spi_init(uint32_t baudrate) {
    spi_init(CARAVEL_SPI, baudrate);
    gpio_set_function(CARAVEL_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CARAVEL_SPI_TX,  GPIO_FUNC_SPI);
    gpio_set_function(CARAVEL_SPI_RX,  GPIO_FUNC_SPI);
    gpio_init(CARAVEL_SPI_CS);
    gpio_set_dir(CARAVEL_SPI_CS, GPIO_OUT);
    gpio_put(CARAVEL_SPI_CS, 1);
}

/* Initialize SPI using the configured default baudrate. */
static inline void caravel_spi_init_default(void) { caravel_spi_init(CARAVEL_SPI_BAUD); }

/* Assert SPI chip select. */
static inline void caravel_spi_select(void) { gpio_put(CARAVEL_SPI_CS, 0); }

/* Deassert SPI chip select. */
static inline void caravel_spi_deselect(void) { gpio_put(CARAVEL_SPI_CS, 1); }

/* Transfer one byte. */
static inline uint8_t caravel_spi_transfer(uint8_t tx) {
    uint8_t rx;
    spi_write_read_blocking(CARAVEL_SPI, &tx, &rx, 1);
    return rx;
}

/* Transfer a buffer. */
static inline void caravel_spi_transfer_blocking(const uint8_t *tx, uint8_t *rx, size_t length) {
    spi_write_read_blocking(CARAVEL_SPI, tx, rx, length);
}

/* Write a buffer. */
static inline void caravel_spi_write(const uint8_t *data, size_t length) {
    spi_write_blocking(CARAVEL_SPI, data, length);
}

/* Read a buffer while transmitting zeroes. */
static inline void caravel_spi_read(uint8_t *data, size_t length) {
    spi_read_blocking(CARAVEL_SPI, 0, data, length);
}

/* ============================================================================
 * I2C
 * ========================================================================== */

/* Initialize I2C. */
static inline void caravel_i2c_init(uint32_t baudrate) {
    i2c_init(CARAVEL_I2C, baudrate);
    gpio_set_function(CARAVEL_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(CARAVEL_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(CARAVEL_I2C_SDA);
    gpio_pull_up(CARAVEL_I2C_SCL);
}

/* Initialize I2C using the configured default baudrate. */
static inline void caravel_i2c_init_default(void) { caravel_i2c_init(CARAVEL_I2C_BAUD); }

/* Write bytes to an I2C device. */
static inline int caravel_i2c_write(uint8_t address, const uint8_t *data, size_t length, bool no_stop) {
    return i2c_write_blocking(CARAVEL_I2C, address, data, length, no_stop);
}

/* Read bytes from an I2C device. */
static inline int caravel_i2c_read(uint8_t address, uint8_t *data, size_t length, bool no_stop) {
    return i2c_read_blocking(CARAVEL_I2C, address, data, length, no_stop);
}

/* ============================================================================
 * UART
 * ========================================================================== */

/* Initialize the Caravel UART. */
static inline void caravel_uart_init(uint32_t baudrate) {
    uart_init(CARAVEL_UART, baudrate);
    gpio_set_function(CARAVEL_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(CARAVEL_UART_RX, GPIO_FUNC_UART);
}

/* Initialize UART using the configured default baudrate. */
static inline void caravel_uart_init_default(void) { caravel_uart_init(CARAVEL_UART_BAUD); }

/* Write one byte. */
static inline void caravel_uart_write_byte(uint8_t data) { uart_putc_raw(CARAVEL_UART, data); }

/* Write a buffer. */
static inline void caravel_uart_write(const uint8_t *data, size_t length) {
    uart_write_blocking(CARAVEL_UART, data, length);
}

/* Return true if UART data is available. */
static inline bool caravel_uart_available(void) { return uart_is_readable(CARAVEL_UART); }

/* Read one byte, returning -1 if no data is available. */
static inline int caravel_uart_read_byte(void) {
    if (!uart_is_readable(CARAVEL_UART)) return -1;
    return uart_getc(CARAVEL_UART);
}

/* ============================================================================
 * ADC
 * ========================================================================== */

/*
 * Initialize ADC2 and ADC3.
 *
 * GPIO28 / ADC2 -> Caravel GP21
 * GPIO29 / ADC3 -> Caravel GP20
 */
static inline void caravel_adc_init(void) {
    adc_init();
    adc_gpio_init(CARAVEL_ADC2_GPIO);
    adc_gpio_init(CARAVEL_ADC3_GPIO);
}

/* Read a raw ADC channel. */
static inline uint16_t caravel_adc_read_raw(uint channel) {
    adc_select_input(channel);
    return adc_read();
}

/* Read ADC2 / Caravel GP21. */
static inline uint16_t caravel_adc2_read(void) { return caravel_adc_read_raw(CARAVEL_ADC2_CHANNEL); }

/* Read ADC3 / Caravel GP20. */
static inline uint16_t caravel_adc3_read(void) { return caravel_adc_read_raw(CARAVEL_ADC3_CHANNEL); }

/*
 * Convert a 12-bit ADC result to millivolts.
 *
 * This assumes the RP2040 ADC input is directly connected to a 3.3 V
 * signal. External resistor dividers must be accounted for by the caller.
 */
static inline uint32_t caravel_adc_raw_to_mv(uint16_t raw) {
    return ((uint32_t)raw * CARAVEL_ADC_VREF_MV) / 4095u;
}

/* Read ADC2 in millivolts. */
static inline uint32_t caravel_adc2_read_mv(void) { return caravel_adc_raw_to_mv(caravel_adc2_read()); }

/* Read ADC3 in millivolts. */
static inline uint32_t caravel_adc3_read_mv(void) { return caravel_adc_raw_to_mv(caravel_adc3_read()); }

/* ============================================================================
 * PWM
 * ========================================================================== */

/*
 * Configure a GPIO for PWM.
 *
 * Returns the PWM slice number.
 */
static inline uint caravel_pwm_init(uint gpio, uint32_t frequency_hz, uint16_t duty_cycle) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint32_t clock_hz = 125000000u;
    uint32_t wrap = clock_hz / frequency_hz;
    if (wrap > 65535u) wrap = 65535u;
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, (uint16_t)(wrap - 1u));
    pwm_init(slice, &config, true);
    pwm_set_gpio_level(gpio, ((uint32_t)(wrap - 1u) * duty_cycle) / 1000u);
    return slice;
}

/*
 * Set PWM duty cycle in thousandths of full scale.
 *
 * 0    = 0%
 * 500  = 50%
 * 1000 = 100%
 */
static inline void caravel_pwm_set_duty(uint gpio, uint16_t duty_cycle) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint16_t wrap = pwm_get_wrap(slice);
    if (duty_cycle > 1000) duty_cycle = 1000;
    pwm_set_gpio_level(gpio, ((uint32_t)wrap * duty_cycle) / 1000u);
}

/* ============================================================================
 * Timing
 * ========================================================================== */

static inline void caravel_delay_us(uint32_t us) { sleep_us(us); }
static inline void caravel_delay_ms(uint32_t ms) { sleep_ms(ms); }
static inline uint64_t caravel_time_us(void) { return time_us_64(); }

/* ============================================================================
 * PIO helpers
 * ========================================================================== */

/*
 * Claim an unused PIO state machine.
 *
 * Returns:
 *   >= 0  = state machine number
 *   -1    = no state machine available
 */
static inline int caravel_pio_claim(PIO pio) {
    for (uint sm = 0; sm < 4; sm++) {
        if (pio_sm_is_claimed(pio, sm)) continue;
        pio_sm_claim(pio, sm);
        return (int)sm;
    }
    return -1;
}

/* Release a previously claimed PIO state machine. */
static inline void caravel_pio_release(PIO pio, uint sm) { pio_sm_unclaim(pio, sm); }

/* ============================================================================
 * DMA helpers
 * ========================================================================== */

/*
 * Claim an unused DMA channel.
 *
 * Returns:
 *   >= 0  = DMA channel
 *   -1    = no channel available
 */
static inline int caravel_dma_claim(void) {
    for (uint channel = 0; channel < NUM_DMA_CHANNELS; channel++) {
        if (dma_channel_is_claimed(channel)) continue;
        dma_channel_claim(channel);
        return (int)channel;
    }
    return -1;
}

/* Release a DMA channel. */
static inline void caravel_dma_release(uint channel) { dma_channel_unclaim(channel); }

/* ============================================================================
 * Common test helpers
 * ========================================================================== */

/* Wait until USB is connected. */
static inline void caravel_usb_wait(void) {
    while (!caravel_usb_connected()) sleep_ms(10);
}

/* Print a simple hexadecimal value over USB. */
static inline void caravel_usb_print_hex(const char *name, uint32_t value) {
    printf("%s = 0x%08lx\n", name, (unsigned long)value);
}

/* Print a simple decimal value over USB. */
static inline void caravel_usb_print_dec(const char *name, uint32_t value) {
    printf("%s = %lu\n", name, (unsigned long)value);
}

/*
 * Pulse an arbitrary Caravel GPIO.
 */
static inline void caravel_gpio_pulse(uint gpio, uint32_t duration_us) {
    gpio_put(gpio, 1);
    sleep_us(duration_us);
    gpio_put(gpio, 0);
}

/*
 * Generate a square wave on a GPIO.
 *
 * Intended for simple test clocks, not high-speed generation.
 */
static inline void caravel_gpio_clock(uint gpio, uint32_t frequency_hz, uint32_t cycles) {
    uint32_t half_period_us = 500000u / frequency_hz;
    caravel_gpio_output(gpio, 0);
    for (uint32_t i = 0; i < cycles; i++) {
        gpio_put(gpio, 1);
        sleep_us(half_period_us);
        gpio_put(gpio, 0);
        sleep_us(half_period_us);
    }
}

#endif /* TEAM_CARAVEL_H */