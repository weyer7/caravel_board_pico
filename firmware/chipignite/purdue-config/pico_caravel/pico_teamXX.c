//pin mappings + commands
#include "pico_includes.h"

//ADD HELPERS BELOW


//ADD HELPERS ABOVE

int main() {
    stdio_init_all(); // Init USB CDC Serial
    //YOUR CODE BELOW

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

    // Initialize the GPIO pin for the LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);
    }

    //YOUR CODE ABOVE
    while (true) {
        tight_loop_contents();
    }
    return 0;
}