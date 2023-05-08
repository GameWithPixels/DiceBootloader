/*
 * Neopixel driver
 */

#ifndef NEOPIXEL_H__
#define NEOPIXEL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void svcs_neopixelInit();
void svcs_neopixelShow(uint32_t* colors, int count);
void svcs_neopixelSetHighestLED(uint32_t colors);
void svcs_neopixelTestLEDReturn();
#ifdef __cplusplus
}
#endif

#endif //NEOPIXEL_H__