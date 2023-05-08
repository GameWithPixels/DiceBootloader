#include "neopixel_bitbang.h"
#include "svcs/svcs_neopixel.h"
#include "svcs/svcs_board_config.h"
#include "compiler_abstraction.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

void powerOn() {
    // Get board
    const struct Board_t* board = svcs_getBoard();

    // Make sure pins are setup correctly, just in case
    nrf_gpio_cfg_output(board->ledPowerPin);

    if (nrf_gpio_pin_out_read(board->ledPowerPin) == 0) {
        // Turn power on
        nrf_gpio_pin_set(board->ledPowerPin);
        // Wait for LEDs to start up
        nrf_delay_ms(5);
    }
}

void powerOff() {
    // Get board
    const struct Board_t* board = svcs_getBoard();
    // Turn power off
    nrf_gpio_pin_clear(board->ledPowerPin);
}

// void setDebugLEDs(uint32_t* colors, int count) {

//     powerOn();
//     svcs_neopixelShow(colors, count);
// }

void SetHighestLED(uint32_t color) {
    powerOn();
    svcs_neopixelSetHighestLED(color);
}

void ClearLEDs() {
    powerOff();
}