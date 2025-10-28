#ifndef BOARD_CONFIG_H__
#define BOARD_CONFIG_H__

#include "stddef.h"
#include "stdint.h"

#pragma pack(push, 1)

#define Board_Unsupported   0
#define Board_Unknown       1
#define Board_D20V15        2
#define Board_D6V4          3
#define Board_D12V2         4
#define Board_PD6V3         5
#define Board_D10V2         6
#define Board_D8V2          7
#define Board_D6V6          8
#define Board_PD6V5         9
#define Board_D6V9          10
#define Board_D00V1         11
#define Board_PyreV1        12
#define Board_D20V17        13
#define Board_D12V7         14
#define Board_D00V2         15
#define Board_D8V8          16
#define Board_D6V10         17
#define Board_D20V18        18
#define Board_D12V8         19
#define Board_D00V3         20
#define Board_D8V9          21
#define Board_D6V11         22

struct Board_t
{
    // Measuring board type
    uint16_t boardResistorValueIn100Ohms;

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

    // A2D Selection Pin
    uint8_t ntcOptSelectPin;

    // Shipping Mode Pin
    uint8_t shippingModePin;

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