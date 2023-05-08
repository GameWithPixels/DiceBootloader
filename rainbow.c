#include "rainbow.h"

// Input a value 0 to 255 to get a color value.
// The colours are a transition g - b - r - back to g.
uint32_t rainbowWheel(uint8_t WheelPos, uint8_t intensity)
{
	if (WheelPos < 85)
	{
		return TO_COLOR(WheelPos * 3 * intensity / 255, (255 - WheelPos * 3) * intensity / 255, 0);
	}
	else if (WheelPos < 170)
	{
		WheelPos -= 85;
		return TO_COLOR((255 - WheelPos * 3) * intensity / 255, 0, WheelPos * 3 * intensity / 255);
	}
	else
	{
		WheelPos -= 170;
		return TO_COLOR(0, WheelPos * 3 * intensity / 255, (255 - WheelPos * 3) * intensity / 255);
	}
}

