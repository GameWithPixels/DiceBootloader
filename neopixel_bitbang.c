#include "neopixel_bitbang.h"
#include "svcs/svcs_neopixel.h"
#include "svcs/svcs_board_config.h"
#include "svcs/svcs_a2d.h"
#include "compiler_abstraction.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "battery.h"
#include "rainbow.h"

#define LOW_BATT_LED_INTENSITY 1

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
    if (getBatteryState() != BatteryState_VeryLow) {
        powerOn();
        if (getBatteryState() == BatteryState_Low) {
            // Clamp intensity
            uint8_t r = GET_R(color);
            if (r > LOW_BATT_LED_INTENSITY) {
                r = LOW_BATT_LED_INTENSITY;
            }
            uint8_t g = GET_G(color);
            if (g > LOW_BATT_LED_INTENSITY) {
                g = LOW_BATT_LED_INTENSITY;
            }
            uint8_t b = GET_B(color);
            if (b > LOW_BATT_LED_INTENSITY) {
                b = LOW_BATT_LED_INTENSITY;
            }
            color = TO_COLOR(r,g,b);
        }
        svcs_neopixelSetHighestLED(color);
    }
}

void SetBinary(int16_t value) {
    uint32_t colors[16];
    for (int i = 0; i < 16; ++i) {
        if ((value & (1 << i)) != 0) {
            colors[i] = 0x008000;
        } else {
            colors[i] = 0x800000;
        }
    }
    svcs_neopixelShow(colors, 16);
}

void BlinkHighestLED(uint32_t color) {
    if (getBatteryState() != BatteryState_VeryLow) {
        powerOn();
        if (getBatteryState() == BatteryState_Low) {
            // Clamp intensity
            uint8_t r = GET_R(color);
            if (r > LOW_BATT_LED_INTENSITY) {
                r = LOW_BATT_LED_INTENSITY;
            }
            uint8_t g = GET_G(color);
            if (g > LOW_BATT_LED_INTENSITY) {
                g = LOW_BATT_LED_INTENSITY;
            }
            uint8_t b = GET_B(color);
            if (b > LOW_BATT_LED_INTENSITY) {
                b = LOW_BATT_LED_INTENSITY;
            }
            color = TO_COLOR(r,g,b);
        }
        for (int i = 0; i < 5; ++i) {
            svcs_neopixelSetHighestLED(color);
            nrf_delay_ms(50);
            svcs_neopixelSetHighestLED(0);
            nrf_delay_ms(10);
        }
        powerOff();
    }
}

void ClearLEDs() {
    powerOff();
}