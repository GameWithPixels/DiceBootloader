#include "svcs_neopixel.h"
#include "nrf_drv_pwm.h"
#include "nrf_delay.h"
#include "svcs/svcs_board_config.h"
#include "nrf_log.h"
#include "svcs.h"

#if defined(PIXELS_BOOTLOADER)
#include "nrf_svc_function.h"
#endif

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

// change this for your settings
#define NEOPIXEL_INSTANCE 0

#define MAX_COUNT 25		// Max LED count so far is 24 (on D6V10)

#define NEOPIXEL_BYTES 24
#define CLOCK NRF_PWM_CLK_16MHz
#define TOP 20
#define DUTY0 6
#define DUTY1 13

//__attribute__((section(".svcs_ram")))
static nrf_drv_pwm_t m_pwm0;
//__attribute__((section(".svcs_ram")))
static nrf_pwm_values_common_t pwm_sequence_values[MAX_COUNT * NEOPIXEL_BYTES + 1];
//__attribute__((section(".svcs_ram")))
static uint8_t numLEDs;
//__attribute__((section(".svcs_ram")))
static uint8_t dataPin;

void writeColor(uint32_t color, uint32_t ledIndex) {

    // Reorder the color bytes to match the hardware
    int pwm_baseIndex = 24 * ledIndex;
    for (int i = 0; i < 8; ++i) {
        pwm_sequence_values[pwm_baseIndex + 16 + i] = ((color & 0x000080) == 0 ? DUTY0 : DUTY1) | 0x8000;
        pwm_sequence_values[pwm_baseIndex +  8 + i] = ((color & 0x800000) == 0 ? DUTY0 : DUTY1) | 0x8000;
        pwm_sequence_values[pwm_baseIndex      + i] = ((color & 0x008000) == 0 ? DUTY0 : DUTY1) | 0x8000;
        color <<= 1;
    }
}

void pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
    // Nothing
}

#else

#include "nrf_sdm.h"

//SVCALL(PIXELS_SVCS_NEOPIXEL_SHOW, void, svcs_neopixelShow_SVC(uint32_t* colors, int count));

#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

void svcs_neopixelInit() {
#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

    m_pwm0.p_registers  = NRFX_CONCAT_2(NRF_PWM, NEOPIXEL_INSTANCE);
    m_pwm0.drv_inst_idx = NRFX_CONCAT_3(NRFX_PWM, NEOPIXEL_INSTANCE, _INST_IDX);

    // Cache configuration data
    const struct Board_t* board = svcs_getBoard();
    dataPin = board->ledDataPin;
    numLEDs = board->ledCount;
    
    nrf_drv_pwm_config_t const config0 =
        {
            .output_pins =
                {
                    dataPin,                  // channel 0
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

// #else
//     // Set the vector table base address to the bootloader
//     ret_code_t err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_ADDRESS);
//     APP_ERROR_CHECK(err_code);

//     svcs_a2dInit_SVC();

//     // Set the vector table base address back to main application.
//     err_code = sd_softdevice_vector_table_base_set(CODE_START);
//     APP_ERROR_CHECK(err_code);
#endif
}

void svcs_neopixelShow(uint32_t* colors, int count) {
#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
    for (int i = 0; i < count; i++) {
        writeColor(colors[i], i);
    }
    // write the termination word
    pwm_sequence_values[count * NEOPIXEL_BYTES] = 0x8000;

    nrf_pwm_sequence_t const seq0 =
        {
            .values = {
                .p_common = pwm_sequence_values,
            },
            .length = (uint16_t)(NEOPIXEL_BYTES * numLEDs + 1),
            .repeats = 0,
            .end_delay = 0};

    (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1, NRF_DRV_PWM_FLAG_STOP);
#else
    // Set the vector table base address to the bootloader
    ret_code_t err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_ADDRESS);
    APP_ERROR_CHECK(err_code);

    svcs_neopixelShow_SVC(colors);

    // Set the vector table base address back to main application.
    err_code = sd_softdevice_vector_table_base_set(CODE_START);
    APP_ERROR_CHECK(err_code);
#endif
}

void svcs_neopixelSetHighestLED(uint32_t color) {
    const struct Board_t* board = svcs_getBoard();
    for (int i = 0; i < board->ledCount; i++) {
        writeColor(0, i);
    }
    writeColor(color, board->debugLedIndex);

    // write the termination word
    pwm_sequence_values[board->ledCount * NEOPIXEL_BYTES] = 0x8000;

    nrf_pwm_sequence_t const seq0 =
        {
            .values = {
                .p_common = pwm_sequence_values,
            },
            .length = (uint16_t)(NEOPIXEL_BYTES * numLEDs + 1),
            .repeats = 0,
            .end_delay = 0};

    (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1, NRF_DRV_PWM_FLAG_STOP);
}

#ifdef PIXELS_BOOTLOADER
//NRF_SVC_FUNCTION_REGISTER(PIXELS_SVCS_NEOPIXEL_SHOW, neopixelShow_instance, svcs_neopixelShow);
#endif
