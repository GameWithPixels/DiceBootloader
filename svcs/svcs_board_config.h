#ifndef SVCS_H__
#define SVCS_H__

#include <stdint.h>

/// @brief The list of Supervisor calls the firmware can make into the bootloader
enum PIXELS_SVCS {
    PIXELS_SVCS_GETBOARD_ID = 1,
};

enum BoardId
{
    Board_Undefined = 0xFF,
    Board_D20V17 = 0,
    Board_D12V7,
    Board_D00V2,
    Board_D8V8,
    Board_D6V10,
    Board_PD6V3,
    Board_D00V3,
    Board_D8V9,
    Board_D6V11,
    Board_D6V6,
    Board_PD6V5,
    Board_D20V15,
    Board_D6V4,
    Board_D12V2,
    Board_D10V2,
    Board_D20V18,
    Board_D8V2,
    Board_D12V8,
    Board_D6V9,
    Board_D00V1,
    Board_PyreV1
};


#ifdef __cplusplus
extern "C" {
#endif
enum BoardId svcs_getBoardId();
#ifdef __cplusplus
}
#endif

#endif //SVCS_H__