#include "svcs_a2d.h"
#include "svcs.h"

#include "nrf_drv_saadc.h"

#if defined(PIXELS_BOOTLOADER)
#include "nrf_svc_function.h"
#endif

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

void nullSAADCHandler(nrfx_saadc_evt_t const * p_event) {
    // Nothing
}

int16_t a2d_readPin(nrf_saadc_input_t pin) {
    // Set A2D pin
    nrf_saadc_channel_config_t channel_config =
    {
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
        .gain       = NRF_SAADC_GAIN1_6,
        .reference  = NRF_SAADC_REFERENCE_INTERNAL,
        .acq_time   = NRF_SAADC_ACQTIME_40US,
        .mode       = NRF_SAADC_MODE_SINGLE_ENDED,
        .burst      = NRF_SAADC_BURST_DISABLED,
        .pin_p      = pin,
        .pin_n      = NRF_SAADC_INPUT_DISABLED
    };
    
    ret_code_t err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    if (err_code == NRF_SUCCESS) {
        int16_t ret;
        err_code = nrf_drv_saadc_sample_convert(0, &ret);
        if (err_code != NRF_SUCCESS) {
            ret = -1;
        }

        nrf_drv_saadc_channel_uninit(0);
        return ret;
    } else {
        return -1;
    }
}

#else

#include "nrf_sdm.h"

SVCALL(PIXELS_SVCS_A2D_READ_PIN_TIMES_1000, int32_t, svcs_a2dReadPinValueTimes1000_SVC(nrf_saadc_input_t pin));

#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

void svcs_a2dInit() {
#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
    nrf_drv_saadc_init(NULL, nullSAADCHandler);
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

int32_t svcs_a2dReadPinValueTimes1000(nrf_saadc_input_t pin) {
#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
    // Digital value read is [V(p) - V(n)] * Gain / Reference * 2^(Resolution - m)
    // In our case:
    // - V(n) = 0
    // - Gain = 1/6
    // - Reference = 0.6V
    // - Resolution = 10
    // - m = 0
    // val = V(p) * 2^12 / (6 * 0.6)
    // => V(p) = val * 3.6 / 2^10
    // => V(p) = val * 0.003515625
    // V(p) * 1,000,000 = val * 3516

    int32_t val = a2d_readPin(pin);
    if (val != -1) {
        return (val * 3516) / 1000;
    } else {
        return 0;
    }
#else
    // Set the vector table base address to the bootloader
    ret_code_t err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_ADDRESS);
    APP_ERROR_CHECK(err_code);

    int32_t ret = svcs_a2dReadPinValueTimes1000_SVC(pin);

    // Set the vector table base address back to main application.
    err_code = sd_softdevice_vector_table_base_set(CODE_START);
    APP_ERROR_CHECK(err_code);
    return ret;
#endif
}

#ifdef PIXELS_BOOTLOADER
NRF_SVC_FUNCTION_REGISTER(PIXELS_SVCS_A2D_READ_PIN_TIMES_1000, a2dReadPinValueTimes1000_instance, svcs_a2dReadPinValueTimes1000);
#endif
