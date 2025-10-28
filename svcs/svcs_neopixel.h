/*
 * Neopixel driver
 */

#ifndef NEOPIXEL_H__
#define NEOPIXEL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
void svcs_neopixelInit();
void svcs_neopixelDeinit();
void svcs_neopixelShow(uint32_t* colors, int count);
void svcs_neopixelSetHighestLED(uint32_t colors);
bool svcs_neopixelTestLEDReturn();
void svcs_neopixelPowerOn();
void svcs_neopixelPowerOff();
#ifdef __cplusplus
}
#endif

#endif //NEOPIXEL_H__