#ifndef BOARD_CONFIG_H__
#define BOARD_CONFIG_H__

#include "stddef.h"
#include "stdint.h"
#include "svcs/svcs_board_config.h"

#pragma pack(push, 1)

#define LED_MAX_COUNT 25

struct Board_t
{
    // Measuring board type
    uint16_t boardResistorValueIn100Ohms;

    // Talking to LEDs
    uint8_t ledDataPin;
    uint8_t ledPowerPin;
    uint8_t ledReturnPin;
    uint8_t debugLedIndex;
    enum BoardId boardId;
};

#pragma pack(pop)

typedef struct Board_t Board;

void boardInit();
const struct Board_t* getBoard();

#endif //BOARD_CONFIG_H__