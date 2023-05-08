/*
 * A2D driver for bootloader / firmware
 */

#ifndef A2D_BOOTLOADER_H__
#define A2D_BOOTLOADER_H__

#include <stdint.h>
#include "nrf_saadc.h"

#ifdef __cplusplus
extern "C" {
#endif
void svcs_a2dInit();
int32_t svcs_a2dReadPinValueTimes1000(nrf_saadc_input_t pin);
#ifdef __cplusplus
}
#endif

#endif //A2D_BOOTLOADER_H__