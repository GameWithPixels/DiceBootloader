/*
 * A2D driver for bootloader / firmware
 */

#ifndef A2D_BOOTLOADER_H__
#define A2D_BOOTLOADER_H__

#include <stdint.h>
#include "nrf_saadc.h"

void a2dInit();
int32_t a2dReadPinValueTimes1000(nrf_saadc_input_t pin);

#endif //A2D_BOOTLOADER_H__