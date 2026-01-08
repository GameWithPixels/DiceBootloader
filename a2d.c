#include "a2d.h"
#include "nrf_drv_saadc.h"

void nullSAADCHandler(nrfx_saadc_evt_t const * p_event) {
    // Nothing
}

void a2dInit() {
    nrf_drv_saadc_init(NULL, nullSAADCHandler);
}

void a2dUninit() {
    nrf_drv_saadc_uninit();
}

int32_t a2dReadPinValueTimes1000(nrf_saadc_input_t pin) {
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
    
    int16_t val;
    nrf_drv_saadc_channel_init(0, &channel_config);
    nrf_drv_saadc_sample_convert(0, &val);
    nrf_drv_saadc_channel_uninit(0);
    return (val * 3516) / 1000;
}

