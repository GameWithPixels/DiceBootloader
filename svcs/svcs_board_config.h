#ifndef BOARD_CONFIG_H__
#define BOARD_CONFIG_H__

#include "stddef.h"
#include "stdint.h"

#pragma pack(push, 1)

enum BoardModel
{
    Unsupported = 0,
    D20BoardV15,
    D6BoardV4,
    D12BoardV2,
    PD6BoardV3,
    D10BoardV2,
    D8BoardV2,
    D6BoardV6,
    PD6BoardV5,
    D6BoardV9,
    D00BoardV1,
};

struct Board_t
{
    // Measuring board type
    uint16_t boardResistorValueInKOhms;

    // Talking to LEDs
    uint8_t ledDataPin;
    uint8_t ledPowerPin;
    uint8_t ledReturnPin;

    // I2C Pins for accelerometer
    uint8_t i2cDataPin;
    uint8_t i2cClockPin;
    uint8_t accInterruptPin;

    // Power Management pins
    uint8_t chargingStatePin;
    uint8_t coilSensePin;
    uint8_t vbatSensePin;
    uint8_t ntcSensePin;
    uint8_t progPin;

    // LED config
    uint8_t ledCount;
    uint8_t debugLedIndex;

    // Board
    uint8_t model;

    // Name of the board
    const char* name;
};

#pragma pack(pop)

typedef struct Board_t Board;

#ifdef __cplusplus
extern "C" {
#endif
void svcs_boardInit();
const struct Board_t* svcs_getBoard();

void svcs_setNTC_ID_VDD();
void svcs_clearNTC_ID_VDD();
#ifdef __cplusplus
}
#endif

#endif //BOARD_CONFIG_H__