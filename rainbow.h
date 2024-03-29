#ifndef RAINBOW_H__
#define RAINBOW_H__

#include <stdint.h>

// Helper function to generate a color
#define GET_R(color) ((color & 0xff0000) >> 16)
#define GET_G(color) ((color & 0x00ff00) >> 8)
#define GET_B(color) ((color & 0x0000ff))
#define TO_COLOR(red, green, blue) ((uint32_t)((red) << 16) | (uint32_t)((green) << 8) | (uint32_t)(blue))
#define BL_LED_INTENSITY 8

// Input a value 0 to 255 to get a color value.
// The colours are a transition g - b - r - back to g.
uint32_t rainbowWheel(uint8_t WheelPos, uint8_t intensity);

#endif //RAINBOW_H__