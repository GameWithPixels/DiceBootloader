/*
 * Monitor battery on start up
 */

#ifndef BATTERY_H__
#define BATTERY_H__

#include <stdint.h>

enum BatteryState
{
    BatteryState_Ok = 0,
    BatteryState_Low,
    BatteryState_VeryLow
};

#ifdef __cplusplus
extern "C" {
#endif
void batteryInit();
enum BatteryState getBatteryState();
#ifdef __cplusplus
}
#endif

#endif //BATTERY_H__