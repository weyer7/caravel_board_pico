//pin mappings + commands
#include "pico_includes.h"

//ADD HELPERS BELOW
#define UART_ID uart0
#define BAUD_RATE 9600

// LED Configuration (Active-Low: 0 = ON, 1 = OFF)

#define LED_ON_MS           200   // Duration LED stays ON per blink cycle
#define LED_OFF_MS          200   // Duration LED stays OFF per blink cycle
#define ACTIVITY_TIMEOUT_MS 150  // How long to keep the blink cycle going after last byte

// TX State Tracking
static bool tx_is_on = false;
static absolute_time_t tx_phase_expiry;
static absolute_time_t tx_activity_expiry;

// RX State Tracking
static bool rx_is_on = false;
static absolute_time_t rx_phase_expiry;
static absolute_time_t rx_activity_expiry;

static void uart_bridge_init(void) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(PIN_UART_TXD, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RXD, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);

    // Initialize LED pins as outputs, default OFF (1 for active-low)
    gpio_init(PIN_LED_TX);
    gpio_set_dir(PIN_LED_TX, GPIO_OUT);
    gpio_put(PIN_LED_TX, 1);

    gpio_init(PIN_LED_RX);
    gpio_set_dir(PIN_LED_RX, GPIO_OUT);
    gpio_put(PIN_LED_RX, 1);

    gpio_init(PIN_LED1);
    gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_put(PIN_LED1, 1);

    gpio_init(PIN_LED2);
    gpio_set_dir(PIN_LED2, GPIO_OUT);
    gpio_put(PIN_LED2, 1);

    tx_phase_expiry = get_absolute_time();
    tx_activity_expiry = get_absolute_time();
    rx_phase_expiry = get_absolute_time();
    rx_activity_expiry = get_absolute_time();
}

static void update_leds(void) {
    absolute_time_t now = get_absolute_time();

    // --- TX LED State Machine ---
    bool tx_active = (absolute_time_diff_us(now, tx_activity_expiry) > 0);

    if (tx_active) {
        // Check if current ON/OFF phase duration has elapsed
        if (absolute_time_diff_us(now, tx_phase_expiry) <= 0) {
            tx_is_on = !tx_is_on; // Flip phase
            gpio_put(PIN_LED_TX, tx_is_on ? 0 : 1); // 0 = ON, 1 = OFF
            
            // Schedule the end of this specific phase
            uint32_t duration = tx_is_on ? LED_ON_MS : LED_OFF_MS;
            tx_phase_expiry = delayed_by_ms(now, duration);
        }
    } else {
        // Idle: Force LED OFF and reset state for the next burst
        tx_is_on = false;
        gpio_put(PIN_LED_TX, 1);
    }

    // --- RX LED State Machine ---
    bool rx_active = (absolute_time_diff_us(now, rx_activity_expiry) > 0);

    if (rx_active) {
        if (absolute_time_diff_us(now, rx_phase_expiry) <= 0) {
            rx_is_on = !rx_is_on;
            gpio_put(PIN_LED_RX, rx_is_on ? 0 : 1);
            
            uint32_t duration = rx_is_on ? LED_ON_MS : LED_OFF_MS;
            rx_phase_expiry = delayed_by_ms(now, duration);
        }
    } else {
        rx_is_on = false;
        gpio_put(PIN_LED_RX, 1);
    }
}
//ADD HELPERS ABOVE

int main() {
    stdio_init_all(); // Init USB CDC Serial
    //YOUR CODE BELOW

    uart_bridge_init();

    // Main bidirectional translation loop
    while (true) {
        absolute_time_t now = get_absolute_time();

        // 1. Translate USB Serial -> UART (Host sent data to Caravel)
        int usb_char = getchar_timeout_us(0);
        if (usb_char != PICO_ERROR_TIMEOUT) {
            uart_putc_raw(UART_ID, (char)usb_char);
            
            // Refresh activity window
            tx_activity_expiry = delayed_by_ms(now, ACTIVITY_TIMEOUT_MS);
            
            // If we were completely idle, kick off the ON phase immediately
            if (!tx_is_on && absolute_time_diff_us(now, tx_phase_expiry) > 0) {
                tx_is_on = true;
                gpio_put(PIN_LED_TX, 0); // Turn ON
                tx_phase_expiry = delayed_by_ms(now, LED_ON_MS);
            }
        }

        // 2. Translate UART -> USB Serial (Caravel sent data to Host)
        if (uart_is_readable(UART_ID)) {
            char uart_char = uart_getc(UART_ID);
            putchar(uart_char);
            fflush(stdout);

            // Refresh activity window
            rx_activity_expiry = delayed_by_ms(now, ACTIVITY_TIMEOUT_MS);

            // Kick off ON phase if idle
            if (!rx_is_on && absolute_time_diff_us(now, rx_phase_expiry) > 0) {
                rx_is_on = true;
                gpio_put(PIN_LED_RX, 0); // Turn ON
                rx_phase_expiry = delayed_by_ms(now, LED_ON_MS);
            }
        }

        // Update blink phases continuously
        update_leds();
    }

    //YOUR CODE ABOVE
    while (true) {
        tight_loop_contents();
    }
    return 0;
}