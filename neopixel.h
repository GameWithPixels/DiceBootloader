/*
 * NeoPixel WS2812B Driver for nRF52 series.
 * Custom bit-bang version for NRF52810
 */

#ifndef NRF_NEOPIXEL_BITBANG_H__
#define NRF_NEOPIXEL_BITBANG_H__

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif
//void setDebugLEDs(uint32_t* colors, int count);
void SetHighestLED(uint32_t color);
void BlinkHighestLED(uint32_t color);
void ClearLEDs();
bool TestLEDReturn();
void NeopixelInit();
void NeopixelDeinit();
#ifdef __cplusplus
}
#endif

#endif //NRF_NEOPIXEL_BITBANG_H__