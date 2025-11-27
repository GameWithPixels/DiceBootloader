#include "svcs_board_config.h"
#include "svcs.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

#if defined(PIXELS_BOOTLOADER)
#include "nrf_svc_function.h"
#endif

#include "nrf_saadc.h"
#include "nrf_log.h"
#include "svcs_a2d.h"
#include "svcs_neopixel.h"

#define BOARD_DETECT_DRIVE_PIN 25
#define BOARD_DETECT_SENSE_PIN NRF_SAADC_INPUT_AIN4
#define BOARD_DETECT_RESISTOR 100000 // 100k

// Array of boards that don't have an identifying resistor and for which we need to use the LED data and return pins
static const struct Board_t unidentifiableBoards[] = {

    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 15,
        .ledPowerPin = 17,
        .ledReturnPin = 12,
        .i2cDataPin = 16,
        .i2cClockPin = 18,
        .accInterruptPin = 20,
        .chargingStatePin = 1,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN1,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 0,
        .ntcOptSelectPin = 11,
        .shippingModePin = 8,
        .ledCount = 20,
        .debugLedIndex = 14,
        .model = Board_D20V17,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D20v17",
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 6,
        .ledPowerPin = 5,
        .ledReturnPin = 15,
        .i2cDataPin = 1,
        .i2cClockPin = 0,
        .accInterruptPin = 9,
        .chargingStatePin = 18,
        .coilSensePin = NRF_SAADC_INPUT_AIN4,
        .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 16,
        .ntcOptSelectPin = 10,
        .shippingModePin = 20,
        .ledCount = 12,
        .debugLedIndex = 6,
        .model = Board_D12V7,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D12v7",
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 9,
        .ledPowerPin = 0,
        .ledReturnPin = 1,
        .i2cDataPin = 28,
        .i2cClockPin = 25,
        .accInterruptPin = 10,
        .chargingStatePin = 14,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 12,
        .ntcOptSelectPin = 6,
        .shippingModePin = 15,
        .ledCount = 19,
        .debugLedIndex = 18,
        .model = Board_D00V2,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D00v2",
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 20,
        .ledPowerPin = 14,
        .ledReturnPin = 11,
        .i2cDataPin = 12,
        .i2cClockPin = 8,
        .accInterruptPin = 15,
        .chargingStatePin = 1,
        .coilSensePin = NRF_SAADC_INPUT_AIN1,
        .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        .ntcSensePin = NRF_SAADC_INPUT_AIN3,
        .progPin = 0,
        .ntcOptSelectPin = 16,
        .shippingModePin = 18,
        .ledCount = 8,
        .debugLedIndex = 5,
        .model = Board_D8V8,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D8v8",
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 25,
        .i2cDataPin = 18,
        .i2cClockPin = 20,
        .accInterruptPin = 16,
        .chargingStatePin = 6,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 9,
        .ntcOptSelectPin = 28,
        .shippingModePin = 10,
        .ledCount = 24,
        .debugLedIndex = 20,
        .model = Board_D6V10,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D6v10",
    },
    {
        .boardResistorValueIn100Ohms = 0, // The resistors are soldered the wrong way, so we measure almost 0 volts.
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
        .model = Board_PD6V3,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "PD6v3-BadR9",
    },
};

// Array of possible circuit boards configs
// Note that the boards MUST be sorted in order of INCREASING resistor value
// for the init method to properly find the correct board config.
static const struct Board_t identifiableBoards[] = {
    {
        .boardResistorValueIn100Ohms = 243, // 24.3k
        .ledDataPin = 6,
        .ledPowerPin = 0,
        .ledReturnPin = 12,
        .i2cDataPin = 10,
        .i2cClockPin = 9,
        .accInterruptPin = 1,
        .chargingStatePin = 14,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 16,
        .ledCount = 19,
        .debugLedIndex = 18,
        .model = Board_D00V3,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D00V3",
    },
    {
        .boardResistorValueIn100Ohms = 330, // 33.0k
        .ledDataPin = 10,
        .ledPowerPin = 18,
        .ledReturnPin = 16,
        .i2cDataPin = 14,
        .i2cClockPin = 12,
        .accInterruptPin = 15,
        .chargingStatePin = 1,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 6,
        .ledCount = 8,
        .debugLedIndex = 5,
        .model = Board_D8V9,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D8V9",
    },
    {
        .boardResistorValueIn100Ohms = 412, // 41.2k
        .ledDataPin = 1,
        .ledPowerPin = 6,
        .ledReturnPin = 0,
        .i2cDataPin = 18,
        .i2cClockPin = 20,
        .accInterruptPin = 16,
        .chargingStatePin = 9,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        .ntcSensePin = NRF_SAADC_INPUT_AIN6,
        .progPin = 12,
        .ledCount = 24,
        .debugLedIndex = 20,
        .model = Board_D6V11,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D6V11",
    },
    {
        .boardResistorValueIn100Ohms = 620, // 62.0k Resistor, at VCC = 3.1V, this means 3.1V * 62k / 162k = 1.18V
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
        .model = Board_D6V6,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D6v6",
    },
    {
        .boardResistorValueIn100Ohms = 800, // 80.0k Resistor, at VCC = 3.1V, this means 3.1V * 80k / 180k = 1.37V
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
        .model = Board_PD6V5,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "PD6v5",
    },
    {
        .boardResistorValueIn100Ohms = 1000, // 100.0k Resistor, at VCC = 3.1V, this means 3.1V * 100k / 200k = 1.55V
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
        .model = Board_D20V15,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D20v15",
    },
    {
        .boardResistorValueIn100Ohms = 1200, // 120.0k Resistor, at VCC = 3.1V, this means 3.1V * 120k / 220k = 1.69V
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
        .model = Board_D6V4,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D6v2",
    },
    {
        .boardResistorValueIn100Ohms = 1500, // 150.0k Resistor, at VCC = 3.1V, this means 3.1V * 150k / 250k = 1.86V
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
        .model = Board_PD6V3,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "PD6v3",
    },
    {
        .boardResistorValueIn100Ohms = 1800, // 180.0k Resistor, at VCC = 3.1V, this means 3.1V * 180k / 280k = 1.99V
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
        .model = Board_D12V2,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D12v2",
    },
    {
        .boardResistorValueIn100Ohms = 2700, // 270.0k Resistor, at VCC = 3.1V, this means 3.1V * 270k / 370k = 2.26V
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
        .model = Board_D10V2,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D10v2",
    },
    {
        .boardResistorValueIn100Ohms = 3480, // 348.0k Resistor
        .ledDataPin = 0,
        .ledPowerPin = 14,
        .ledReturnPin = 1,
        .i2cDataPin = 9,
        .i2cClockPin = 10,
        .accInterruptPin = 6,
        .chargingStatePin = 20,
        .coilSensePin = NRF_SAADC_INPUT_AIN6,
        .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 18,
        .ledCount = 20,
        .debugLedIndex = 14,
        .model = Board_D20V18,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D20v18",
    },
    {
        .boardResistorValueIn100Ohms = 4700,
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
        .model = Board_D8V2,
        .accelerometer = AccelerometerModel_KXTJ3,
        .name = "D8v2",
    },
    {
        .boardResistorValueIn100Ohms = 6810,
        .ledDataPin = 6,
        .ledPowerPin = 10,
        .ledReturnPin = 14,
        .i2cDataPin = 1,
        .i2cClockPin = 0,
        .accInterruptPin = 9,
        .chargingStatePin = 18,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 16,
        .ledCount = 12,
        .debugLedIndex = 6,
        .model = Board_D12V8,
        .accelerometer = AccelerometerModel_SC7A20H,
        .name = "D12v8",
    },
};

static const Board* currentBoard
         __attribute__((section(".current_board_ptr")))
         __attribute__((used))
         = NULL;

void svcs_setNTC_ID_VDD() {
    nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
    nrf_gpio_pin_set(BOARD_DETECT_DRIVE_PIN);
}

void svcs_clearNTC_ID_VDD() {
    nrf_gpio_cfg_default(BOARD_DETECT_DRIVE_PIN);
}


void svcs_boardInit() {

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

    // Check for boards that don't have ID resistor by testing the LED pins
    bool gpioInit = nrf_drv_gpiote_is_init();
    if (!gpioInit) {
        ret_code_t err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    const int unidentifableBoardCount = sizeof(unidentifiableBoards) / sizeof(unidentifiableBoards[0]);
    bool foundBoard = false;
    for (int i = 0; !foundBoard && (i < unidentifableBoardCount); ++i) {

        // Set the pin numbers according to the board we're testing for
        currentBoard = &(unidentifiableBoards[i]);

        if (nrf_gpio_pin_present_check(currentBoard->ledDataPin) &&
            nrf_gpio_pin_present_check(currentBoard->ledPowerPin) &&
            nrf_gpio_pin_present_check(currentBoard->ledReturnPin)) {

            NRF_LOG_INFO("Testing %d: %s!", i, unidentifiableBoards[i].name);

            svcs_neopixelInit();

            // Run the test
            foundBoard = svcs_neopixelTestLEDReturn();

            // Deinit
            svcs_neopixelDeinit();

        }
    }

    if (!gpioInit) {
        nrf_drv_gpiote_uninit();
    }

    if (foundBoard) {
        NRF_LOG_INFO("Board is %s", currentBoard->name);
    } else {
        // We did not find the board by checking the LED data/return pins, try the A2D method

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
        const int boardCount = sizeof(identifiableBoards) / sizeof(identifiableBoards[0]);
        int32_t boardVoltagesTimes1000[boardCount];
        for (int i = 0; i < boardCount; ++i) {
            boardVoltagesTimes1000[i] = (vddTimes1000 * identifiableBoards[i].boardResistorValueIn100Ohms * 100) / (BOARD_DETECT_RESISTOR + identifiableBoards[i].boardResistorValueIn100Ohms * 100);
            NRF_LOG_DEBUG("%s: voltage: %d.%03d", identifiableBoards[i].name, boardVoltagesTimes1000[i] / 1000, boardVoltagesTimes1000[i] % 1000);
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
        currentBoard = &(identifiableBoards[boardIndex]);
        NRF_LOG_INFO("Board is %s, boardId V: %d.%03d", currentBoard->name, vboardTimes1000 / 1000, vboardTimes1000 % 1000);
    }


#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
}


#else

#include "nrf_sdm.h"

SVCALL(PIXELS_SVCS_GETBOARD, const struct Board_t*, svcs_getBoard_SVC());

#endif //defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)

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
NRF_SVC_FUNCTION_REGISTER(PIXELS_SVCS_GETBOARD, getBoard_instance, svcs_getBoard);
#endif
