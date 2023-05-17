#include "battery.h"
#include "svcs/svcs_board_config.h"
#include "svcs/svcs_a2d.h"

// Voltage divider 10M over 4M
#define VBAT_MULT_TIMES_1000 1400


static enum BatteryState currentBatteryState = BatteryState_Ok;
#define BATTERY_LOW_MV      3500
#define BATTERY_VERY_LOW_MV 3200

void batteryInit() {
    // Measure VBat!
    const struct Board_t* board = svcs_getBoard();
    int32_t vbat = svcs_a2dReadPinValueTimes1000(board->vbatSensePin) * VBAT_MULT_TIMES_1000 / 1000;

    if (vbat > BATTERY_LOW_MV) {
        currentBatteryState = BatteryState_Ok;
    } else if (vbat > BATTERY_VERY_LOW_MV) {
        currentBatteryState = BatteryState_Low;
    } else {
        currentBatteryState = BatteryState_VeryLow;
    }
}

enum BatteryState getBatteryState() {
    return currentBatteryState;
}
