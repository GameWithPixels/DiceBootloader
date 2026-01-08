#include "neopixel.h"
#include "nrf_drv_pwm.h"
#include "nrf_log.h"
#include "compiler_abstraction.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "board_config.h"

#define LOW_BATT_LED_INTENSITY 1

// change this for your settings
#define NEOPIXEL_INSTANCE 0
#define NEOPIXEL_BYTES 24
#define CLOCK NRF_PWM_CLK_16MHz
#define TOP 20
#define DUTY0 (6 | 0x8000)
#define DUTY1 (13 | 0x8000)
#define TERMINATOR 0x8000

static nrf_drv_pwm_t m_pwm0;
static nrf_pwm_values_common_t pwm_sequence_values[LED_MAX_COUNT * NEOPIXEL_BYTES + 1];

void pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
    // Nothing
}

void powerOn() {
    // Get board
    const struct Board_t* board = getBoard();
    // Make sure pins are setup correctly, just in case
    nrf_gpio_cfg_output(board->ledPowerPin);
    // Turn power on
    nrf_gpio_pin_set(board->ledPowerPin);
    // Wait for LEDs to start up
    nrf_delay_ms(5);
}

void powerOff() {
    // Get board
    const struct Board_t* board = getBoard();
    // Make sure pins are setup correctly, just in case
    nrf_gpio_cfg_output(board->ledPowerPin);
    // Turn power off
    nrf_gpio_pin_clear(board->ledPowerPin);
    // // Reset pin state
    // nrf_gpio_cfg_input(board->ledPowerPin, NRF_GPIO_PIN_NOPULL);
}


void internalNeopixelSetHighestLED(uint32_t color) {
    const struct Board_t* board = getBoard();

        for (int i = 0; i < sizeof(pwm_sequence_values)/sizeof(uint16_t); ++i) {
        pwm_sequence_values[i] = DUTY0;
    }

    // Reorder the color bytes to match the hardware
    int pwm_baseIndex = 24 * board->debugLedIndex;
    for (int i = 0; i < 8; ++i) {
        if (color & 0x000080) {
            pwm_sequence_values[pwm_baseIndex + 16 + i] = DUTY1;
        }
        if (color & 0x800000) {
            pwm_sequence_values[pwm_baseIndex + 8 + i] = DUTY1;
        }
        if (color & 0x008000) {
            pwm_sequence_values[pwm_baseIndex + 0 + i] = DUTY1;
        }
        color <<= 1;
    }

    // write the termination word
    pwm_sequence_values[LED_MAX_COUNT * NEOPIXEL_BYTES] = TERMINATOR;

    nrf_pwm_sequence_t const seq0 =
        {
            .values = {
                .p_common = pwm_sequence_values,
            },
            .length = (uint16_t)(NEOPIXEL_BYTES * LED_MAX_COUNT + 1),
            .repeats = 0,
            .end_delay = 0};

    (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1, NRF_DRV_PWM_FLAG_STOP);
}


static int ledReturnDetectedCount;
void ledReturnDetector(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    ledReturnDetectedCount++;
}

bool TestLEDReturn() {
    
    nrf_drv_gpiote_in_config_t in_config;
    in_config.is_watcher = false;
    in_config.hi_accuracy = true;
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    in_config.sense = NRF_GPIOTE_POLARITY_TOGGLE;
    in_config.skip_gpio_setup = false;

    uint32_t ledReturnPin = getBoard()->ledReturnPin;
    uint32_t ledDataPin = getBoard()->ledDataPin;

    ret_code_t err_code = nrf_drv_gpiote_in_init(ledReturnPin, &in_config, ledReturnDetector);
    APP_ERROR_CHECK(err_code);

    // Turn interrupt on
    nrf_drv_gpiote_in_event_enable(ledReturnPin, true);

    // Reset data pin
    nrf_gpio_cfg_default(ledDataPin);
    
    NeopixelInit();
    
    // test LED return
    powerOn();

    // Have LEDs test the return
    ledReturnDetectedCount = 0;
    internalNeopixelSetHighestLED(0);

    // Wait (synchronously because we not fancy like that)
    nrf_delay_ms(5);

    // Turn off LED power
    powerOff();

    // Deinit
    NeopixelDeinit();

    // Turn off the interrupt handler
    nrfx_gpiote_in_uninit(ledReturnPin);    
    nrf_gpio_cfg_default(ledReturnPin);

    return ledReturnDetectedCount != 0;
}


void NeopixelInit() {
    m_pwm0.p_registers  = NRFX_CONCAT_2(NRF_PWM, NEOPIXEL_INSTANCE);
    m_pwm0.drv_inst_idx = NRFX_CONCAT_3(NRFX_PWM, NEOPIXEL_INSTANCE, _INST_IDX);

    // Cache configuration data
    const struct Board_t* board = getBoard();
    
    nrf_drv_pwm_config_t const config0 =
        {
            .output_pins =
                {
                    board->ledDataPin,                  // channel 0
                    NRF_DRV_PWM_PIN_NOT_USED, // channel 1
                    NRF_DRV_PWM_PIN_NOT_USED, // channel 2
                    NRF_DRV_PWM_PIN_NOT_USED  // channel 3
                },
            .irq_priority = APP_IRQ_PRIORITY_LOWEST,
            .base_clock = CLOCK,
            .count_mode = NRF_PWM_MODE_UP,
            .top_value = TOP,
            .load_mode = NRF_PWM_LOAD_COMMON,
            .step_mode = NRF_PWM_STEP_AUTO};
    APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, pwm_handler));

    NRF_LOG_DEBUG("Neopixel init");
}

void NeopixelDeinit() {
    nrf_drv_pwm_uninit(&m_pwm0);
    const struct Board_t* board = getBoard();
    nrf_gpio_cfg_default(board->ledPowerPin);
}


void SetHighestLED(uint32_t color) {
    powerOn();
    internalNeopixelSetHighestLED(color);
}

void BlinkHighestLED(uint32_t color) {
    powerOn();
    for (int i = 0; i < 5; ++i) {
        internalNeopixelSetHighestLED(color);
        nrf_delay_ms(50);
        internalNeopixelSetHighestLED(0);
        nrf_delay_ms(10);
    }
    powerOff();
}

void ClearLEDs() {
    powerOff();
}

