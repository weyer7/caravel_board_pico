//pin mappings + commands
#include "pico_includes.h"

//ADD HELPERS BELOW
#define UART_ID uart0
#define BAUD_RATE 9600

// LED Configuration (Active-Low: 0 = ON, 1 = OFF)

#define LED_ON_MS   200
#define LED_OFF_MS  200

typedef struct {
    uint pin;
    bool on;
    bool active;
    bool pending;
    absolute_time_t phase_expiry;
} activity_led_t;

static activity_led_t tx_led;
static activity_led_t rx_led;

static void activity_led_init(activity_led_t *led, uint pin)
{
    led->pin = pin;
    led->on = false;
    led->active = false;
    led->pending = false;
    led->phase_expiry = get_absolute_time();

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 1);  // active-low, OFF
}

static void activity_led_trigger(activity_led_t *led)
{
    absolute_time_t now = get_absolute_time();

    if (!led->active) {
        // Start a complete ON -> OFF cycle.
        led->active = true;
        led->pending = false;
        led->on = true;

        gpio_put(led->pin, 0);
        led->phase_expiry = delayed_by_ms(now, LED_ON_MS);
    } else {
        // Current cycle is already running.
        // Queue exactly one more cycle.
        led->pending = true;
    }
}

static void activity_led_update(activity_led_t *led, absolute_time_t now)
{
    if (!led->active)
        return;

    if (absolute_time_diff_us(now, led->phase_expiry) > 0)
        return;

    if (led->on) {
        // ON -> OFF. This OFF phase is mandatory.
        led->on = false;
        gpio_put(led->pin, 1);
        led->phase_expiry = delayed_by_ms(
            led->phase_expiry, LED_OFF_MS
        );
    } else {
        // OFF phase completed.
        if (led->pending) {
            // Start another complete cycle.
            led->pending = false;
            led->on = true;

            gpio_put(led->pin, 0);
            led->phase_expiry = delayed_by_ms(
                led->phase_expiry, LED_ON_MS
            );
        } else {
            // No more traffic. Return to idle.
            led->active = false;
        }
    }
}

static void update_leds(void)
{
    absolute_time_t now = get_absolute_time();

    activity_led_update(&tx_led, now);
    activity_led_update(&rx_led, now);
}

static void uart_bridge_init(void)
{
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(PIN_UART_TXD, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RXD, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);

    activity_led_init(&tx_led, PIN_LED_TX);
    activity_led_init(&rx_led, PIN_LED_RX);

    gpio_init(PIN_LED1);
    gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_put(PIN_LED1, 1);

    gpio_init(PIN_LED2);
    gpio_set_dir(PIN_LED2, GPIO_OUT);
    gpio_put(PIN_LED2, 1);
}
//ADD HELPERS ABOVE

int main()
{
    stdio_init_all();
    uart_bridge_init();

    while (true) {
        // USB -> Caravel UART
        int usb_char = getchar_timeout_us(0);
        if (usb_char != PICO_ERROR_TIMEOUT) {
            uart_putc_raw(UART_ID, (char)usb_char);
            activity_led_trigger(&tx_led);
        }

        // Caravel UART -> USB
        if (uart_is_readable(UART_ID)) {
            char uart_char = uart_getc(UART_ID);
            putchar(uart_char);
            fflush(stdout);

            activity_led_trigger(&rx_led);
        }

        update_leds();
    }

    return 0;
}