#ifndef SVCS_H__
#define SVCS_H__

#include <stdint.h>
#include "nrf_saadc.h"

/// @brief The list of Supervisor calls the firmware can make into the bootloader
enum PIXELS_SVCS {
    PIXELS_SVCS_GETBOARD = 2,
    PIXELS_SVCS_A2D_READ_PIN_TIMES_1000,
    PIXELS_SVCS_NEOPIXEL_SHOW,
    PIXELS_SVCS_NEOPIXEL_TEST,
};


#endif //SVCS_H__