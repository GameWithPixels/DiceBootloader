#include "svcs_board_config.h"
#include "svcs.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#define BOARD_DETECT_DRIVE_PIN 25

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

#include "nrf_saadc.h"
#include "nrf_log.h"
#include "svcs_a2d.h"

#if defined(PIXELS_BOOTLOADER)
#include "nrf_svc_function.h"
#endif

#define BOARD_DETECT_SENSE_PIN NRF_SAADC_INPUT_AIN4
#define BOARD_DETECT_RESISTOR 100000 // 100k

// Array of possible circuit boards configs
// Note that the boards MUST be sorted in order of INCREASING resistor value
// for the init method to properly find the correct board config.
static const struct Board_t boards[] = {
    {
        .boardResistorValueInKOhms = 0, // The resistors are soldered the wrong way, so we measure almost 0 volts.
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .i2cDataPin = 14,
        .i2cClockPin = 15,
        .accInterruptPin = 12,
        .chargingStatePin = 6,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 0xFF,
        .ledCount = 21,
        .debugLedIndex = 20,
        .model = PD6BoardV3,
        .name = "PD6v3-BadR9",
    },
    {
        .boardResistorValueInKOhms = 100, // 100.0k Resistor, at VCC = 3.1V, this means 3.1V * 100k / 200k = 1.55V
        .ledDataPin = 6,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .i2cDataPin = 15,
        .i2cClockPin = 14,
        .accInterruptPin = 12,
        .chargingStatePin = 1,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 9,
        .ledCount = 20,
        .debugLedIndex = 14,
        .model = D20BoardV15,
        .name = "D20v15",
    },
    {
        .boardResistorValueInKOhms = 120, // 120.0k Resistor, at VCC = 3.1V, this means 3.1V * 120k / 220k = 1.69V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10  ,
        .i2cDataPin = 14,
        .i2cClockPin = 15,
        .accInterruptPin = 12,
        .chargingStatePin = 6,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 0xFF,
        .ledCount = 6,
        .debugLedIndex = 3,
        .model = D6BoardV4,
        .name = "D6v2",
    },
    {
        .boardResistorValueInKOhms = 150, // 150.0k Resistor, at VCC = 3.1V, this means 3.1V * 150k / 250k = 1.86V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .i2cDataPin = 14,
        .i2cClockPin = 15,
        .accInterruptPin = 12,
        .chargingStatePin = 6,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 0xFF,
        .ledCount = 21,
        .debugLedIndex = 20,
        .model = PD6BoardV3,
        .name = "PD6v3",
    },
    {
        .boardResistorValueInKOhms = 180, // 180.0k Resistor, at VCC = 3.1V, this means 3.1V * 180k / 280k = 1.99V
        .ledDataPin = 6,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .i2cDataPin = 15,
        .i2cClockPin = 14,
        .accInterruptPin = 12,
        .chargingStatePin = 1,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 0xFF,
        .ledCount = 12,
        .debugLedIndex = 6,
        .model = D12BoardV2,
        .name = "D12v1",
    },
    {
        .boardResistorValueInKOhms = 270, // 270.0k Resistor, at VCC = 3.1V, this means 3.1V * 270k / 370k = 2.26V
        .ledDataPin = 0,
        .ledPowerPin = 10,
        .ledReturnPin = 1,
        .i2cDataPin = 14,
        .i2cClockPin = 15,
        .accInterruptPin = 12,
        .chargingStatePin = 6,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 0xFF,
        .ledCount = 10,
        .debugLedIndex = 9,
        .model = D10BoardV2,
        .name = "D10v1",
    },
    {
        .boardResistorValueInKOhms = 470,
        .ledDataPin = 9,
        .ledPowerPin = 6,
        .ledReturnPin = 10,
        .i2cDataPin = 14,
        .i2cClockPin = 15,
        .accInterruptPin = 12,
        .chargingStatePin = 0,
        .coilSensePin = NRF_SAADC_INPUT_AIN2,
        .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        .ntcSensePin = NRF_SAADC_INPUT_AIN3,
        .progPin = 0xFF,
        .ledCount = 8,
        .debugLedIndex = 4,
        .model = D8BoardV2,
        .name = "D8v1",
    },
};

static const struct Board_t* currentBoard;

#else

#include "nrf_sdm.h"

//SVCALL(PIXELS_SVCS_GETBOARD, const struct Board_t*, svcs_getBoard_SVC());

#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

void svcs_boardInit() {

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

    // Sample adc board pin
    svcs_setNTC_ID_VDD();

    nrf_delay_ms(5);

    int32_t vboardTimes1000 = svcs_a2dReadPinValueTimes1000((nrf_saadc_input_t)(BOARD_DETECT_SENSE_PIN));

    // Now that we're done reading, we can turn off the drive pin
    svcs_clearNTC_ID_VDD();

    // Do some computation to figure out which variant we're working with!
    // D20v3 board uses 20k over 100k voltage divider
    // i.e. the voltage read should be 3.1V * 20k / 120k = 0.55V
    // The D6v2 board uses 47k over 100k, i.e. 1.05V
    // The D20v2 board should read 0 (unconnected)
    // So we can allow a decent
    const int32_t vddTimes1000 = svcs_a2dReadPinValueTimes1000(NRF_SAADC_INPUT_VDD);

    // Compute board voltages
    const int boardCount = sizeof(boards) / sizeof(boards[0]);
    int32_t boardVoltagesTimes1000[boardCount];
    for (int i = 0; i < boardCount; ++i) {
        boardVoltagesTimes1000[i] = (vddTimes1000 * boards[i].boardResistorValueInKOhms * 1000) / (BOARD_DETECT_RESISTOR + boards[i].boardResistorValueInKOhms * 1000);
        NRF_LOG_DEBUG("%s: voltage: %d.%03d", boards[i].name, boardVoltagesTimes1000[i] / 1000, boardVoltagesTimes1000[i] % 1000);
    }
    int32_t midpointVoltagesTimes1000[boardCount-1];
    for (int i = 0; i < boardCount-1; ++i) {
        midpointVoltagesTimes1000[i] = (boardVoltagesTimes1000[i] + boardVoltagesTimes1000[i+1]) / 2;
    }

    // Find the first midpoint voltage that is above the measured voltage
    int boardIndex = 0;
    for (; boardIndex < boardCount-1; ++boardIndex) {
        if (midpointVoltagesTimes1000[boardIndex] > vboardTimes1000)
        break;
    }
    currentBoard = &(boards[boardIndex]);
    NRF_LOG_INFO("Board is %s, boardId V: %d.%03d", currentBoard->name, vboardTimes1000 / 1000, vboardTimes1000 % 1000);

#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
}

// The board we're currently using
const struct Board_t* svcs_getBoard() {

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
    return currentBoard;
#else //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
    // Set the vector table base address to the bootloader
    ret_code_t err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_ADDRESS);
    APP_ERROR_CHECK(err_code);

    const struct Board_t* board = svcs_getBoard_SVC();

    // Set the vector table base address back to main application.
    err_code = sd_softdevice_vector_table_base_set(CODE_START);
    APP_ERROR_CHECK(err_code);                    

    return board;
#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

}

#ifdef PIXELS_BOOTLOADER
//NRF_SVC_FUNCTION_REGISTER(PIXELS_SVCS_GETBOARD, getBoard_instance, svcs_getBoard);
#endif

void svcs_setNTC_ID_VDD() {
    nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
    nrf_gpio_pin_set(BOARD_DETECT_DRIVE_PIN);
}

void svcs_clearNTC_ID_VDD() {
    nrf_gpio_cfg_default(BOARD_DETECT_DRIVE_PIN);
}
