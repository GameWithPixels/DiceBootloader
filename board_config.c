#include "board_config.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"

#include "nrf_saadc.h"
#include "nrf_log.h"
#include "a2d.h"
#include "neopixel.h"
#include "nrf_power.h"

#define BOARD_DETECT_DRIVE_PIN 25
#define BOARD_DETECT_SENSE_PIN NRF_SAADC_INPUT_AIN4
#define BOARD_DETECT_RESISTOR 100000 // 100k
#define BOARD_ID_VOLTAGE_THRESHOLD 100 //mV

// Array of boards that don't have an identifying resistor and for which we need to use the LED data and return pins
static const struct Board_t unidentifiableBoards[] = {
   {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 15,
        .ledPowerPin = 17,
        .ledReturnPin = 12,
        .debugLedIndex = 14,
        .boardId = Board_D20V17,
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 6,
        .ledPowerPin = 5,
        .ledReturnPin = 15,
        .debugLedIndex = 6,
        .boardId = Board_D12V7,
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 9,
        .ledPowerPin = 0,
        .ledReturnPin = 1,
        .debugLedIndex = 18,
        .boardId = Board_D00V2,
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 20,
        .ledPowerPin = 14,
        .ledReturnPin = 11,
        .debugLedIndex = 5,
        .boardId = Board_D8V8,
    },
    {
        .boardResistorValueIn100Ohms = 0,
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 25,
        .debugLedIndex = 20,
        .boardId = Board_D6V10,
    },
    // CONFLICT WITH PD6V5
    // {
    //     .boardResistorValueIn100Ohms = 0, // The resistors are soldered the wrong way, so we measure almost 0 volts.
    //     .ledDataPin = 1,
    //     .ledPowerPin = 0,
    //     .ledReturnPin = 10,
    //     .debugLedIndex = 20,
    //     .boardId = Board_PD6V3,
    // },
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
        .debugLedIndex = 18,
        .boardId = Board_D00V3,
    },
    {
        .boardResistorValueIn100Ohms = 330, // 33.0k
        .ledDataPin = 10,
        .ledPowerPin = 18,
        .ledReturnPin = 16,
        .debugLedIndex = 5,
        .boardId = Board_D8V9,
    },
    {
        .boardResistorValueIn100Ohms = 412, // 41.2k
        .ledDataPin = 1,
        .ledPowerPin = 6,
        .ledReturnPin = 0,
        .debugLedIndex = 20,
        .boardId = Board_D6V11,
    },
    {
        .boardResistorValueIn100Ohms = 620, // 62.0k Resistor, at VCC = 3.1V, this means 3.1V * 62k / 162k = 1.18V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .debugLedIndex = 3,
        .boardId = Board_D6V6,
    },
    {
        .boardResistorValueIn100Ohms = 800, // 80.0k Resistor, at VCC = 3.1V, this means 3.1V * 80k / 180k = 1.37V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .debugLedIndex = 20,
        .boardId = Board_PD6V5,
    },
    {
        .boardResistorValueIn100Ohms = 1000, // 100.0k Resistor, at VCC = 3.1V, this means 3.1V * 100k / 200k = 1.55V
        .ledDataPin = 6,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .debugLedIndex = 14,
        .boardId = Board_D20V15,
    },
    {
        .boardResistorValueIn100Ohms = 1200, // 120.0k Resistor, at VCC = 3.1V, this means 3.1V * 120k / 220k = 1.69V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10  ,
        .debugLedIndex = 3,
        .boardId = Board_D6V4,
    },
    {
        .boardResistorValueIn100Ohms = 1500, // 150.0k Resistor, at VCC = 3.1V, this means 3.1V * 150k / 250k = 1.86V
        .ledDataPin = 1,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .debugLedIndex = 20,
        .boardId = Board_PD6V3,
    },
    {
        .boardResistorValueIn100Ohms = 1800, // 180.0k Resistor, at VCC = 3.1V, this means 3.1V * 180k / 280k = 1.99V
        .ledDataPin = 6,
        .ledPowerPin = 0,
        .ledReturnPin = 10,
        .debugLedIndex = 6,
        .boardId = Board_D12V2,
    },
    {
        .boardResistorValueIn100Ohms = 2700, // 270.0k Resistor, at VCC = 3.1V, this means 3.1V * 270k / 370k = 2.26V
        .ledDataPin = 0,
        .ledPowerPin = 10,
        .ledReturnPin = 1,
        .debugLedIndex = 9,
        .boardId = Board_D10V2,
    },
    {
        .boardResistorValueIn100Ohms = 3480, // 348.0k Resistor
        .ledDataPin = 0,
        .ledPowerPin = 14,
        .ledReturnPin = 1,
        .debugLedIndex = 14,
        .boardId = Board_D20V18,
    },
    {
        .boardResistorValueIn100Ohms = 4700,
        .ledDataPin = 9,
        .ledPowerPin = 6,
        .ledReturnPin = 10,
        .debugLedIndex = 4,
        .boardId = Board_D8V2,
    },
    {
        .boardResistorValueIn100Ohms = 6810,
        .ledDataPin = 6,
        .ledPowerPin = 10,
        .ledReturnPin = 14,
        .debugLedIndex = 6,
        .boardId = Board_D12V8,
    },
};

static const Board* currentBoard
         __attribute__((section(".current_board_ptr")))
         __attribute__((used))
         = NULL;

void setNTC_ID_VDD() {
    nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
    nrf_gpio_pin_set(BOARD_DETECT_DRIVE_PIN);
}

void gndNTC_ID_VDD() {
    nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
    nrf_gpio_pin_clear(BOARD_DETECT_DRIVE_PIN);
}

void clearNTC_ID_VDD() {
    nrf_gpio_cfg_default(BOARD_DETECT_DRIVE_PIN);
}


void boardInit() {

    // Enable internal DC-DC regulator. This seems to make the softdevice happier, although I have no clue why
    // (it's better to have it enabled, but the chip doesn't *need* to have it enabled).
    // Without the DC-DC regulator enabled, the softdevice would cause a brownout reset while advertising.
    NRF_POWER->DCDCEN = 1;

    // Need to wait about 50ms for VBat sense voltage to reach equilibrium after power-on
    nrf_delay_ms(50);

    // Check for boards that don't have ID resistor by testing the LED pins
    ret_code_t err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // We did not find the board by checking the LED data/return pins, try the A2D method
    a2dInit();

    // Make sure we can sample ID pin
    setNTC_ID_VDD();
    nrf_delay_ms(5);

    int32_t vboardTimes1000 = a2dReadPinValueTimes1000(BOARD_DETECT_SENSE_PIN);
    NRF_LOG_DEBUG("Vboard=%d", vboardTimes1000);

    // Now that we're done reading, we can turn off the drive pin
    clearNTC_ID_VDD();

    // Do some computation to figure out which variant we're working with!
    // D20v3 board uses 20k over 100k voltage divider
    // i.e. the voltage read should be 3.1V * 20k / 120k = 0.55V
    // The D6v2 board uses 47k over 100k, i.e. 1.05V
    // The D20v2 board should read 0 (unconnected)
    // So we can allow a decent
    const int32_t vddTimes1000 = a2dReadPinValueTimes1000(NRF_SAADC_INPUT_VDD);

    // Compute board voltages
    const int identifiableBoardCount = sizeof(identifiableBoards) / sizeof(identifiableBoards[0]);

    int absSmallestVoltageDeltaTimes1000 = BOARD_ID_VOLTAGE_THRESHOLD; // max 100 mV delta
    int boardIndex = -1;
    for (int i = 0; i < identifiableBoardCount; ++i) {
        int boardVoltageTimes1000 = (vddTimes1000 * identifiableBoards[i].boardResistorValueIn100Ohms * 100) / (BOARD_DETECT_RESISTOR + identifiableBoards[i].boardResistorValueIn100Ohms * 100);
        int absDelta = boardVoltageTimes1000 - vboardTimes1000;
        if (absDelta < 0) {
            absDelta = -absDelta;
        }
        if (absDelta < absSmallestVoltageDeltaTimes1000) {
            absSmallestVoltageDeltaTimes1000 = absDelta;
            boardIndex = i;
        }
    }

    bool foundBoard = boardIndex != -1;
    if (foundBoard) {
        //NRF_LOG_INFO("Board is %s, boardId V: %d.%03d", currentBoard->name, vboardTimes1000 / 1000, vboardTimes1000 % 1000);
        currentBoard = &(identifiableBoards[boardIndex]);
    }

    a2dUninit();

    if (!foundBoard) {
        NRF_LOG_WARNING("Bad VBoard, checking LED pins to figure out board model...");
        // Iterate through the unidentifiable boards and try to wiggle the LED data line, see if it's the correct ont
        // None of the other boards have the same LED lines so if we find a match here we're done.
        const int unidentifableBoardCount = sizeof(unidentifiableBoards) / sizeof(unidentifiableBoards[0]);
        for (int i = 0; !foundBoard && (i < unidentifableBoardCount); ++i) {

            // Set the pin numbers according to the board we're testing for
            currentBoard = &(unidentifiableBoards[i]);

            if (nrf_gpio_pin_present_check(currentBoard->ledDataPin) &&
                nrf_gpio_pin_present_check(currentBoard->ledPowerPin) &&
                nrf_gpio_pin_present_check(currentBoard->ledReturnPin)) {

                // Run the test
                foundBoard = TestLEDReturn();
            }
        }
    }

    nrf_drv_gpiote_uninit();

    if (foundBoard) {
        NRF_LOG_DEBUG("Board Identified!");

        // Display reset reason bits
        uint32_t resetReas = nrf_power_resetreas_get();
        nrf_power_resetreas_clear(0xFFFFFFFF);

        static const uint32_t blinkColors[] = {
            0x000400, // RESET PIN -> GREEN
            0x040000, // WATCHDOG -> RED
            0x000004, // SYSTEM REQUEST -> BLUE
            0x040400  // LOCKUP -> YELLOW
        };

        if (resetReas != 0) {
            NeopixelInit();

            for (int i = 0; i < 4; ++i) {
                if ((resetReas & (1 << i)) != 0) {
                    BlinkHighestLED(blinkColors[i]);
                }
            }

            #if DEBUG
            // RESET PIN -> GREEN
            if ((resetReas & (1 << 0)) != 0) {
                NRF_LOG_WARNING("Reset Reason - %s", "PIN RESET");
            }
            // WATCHDOG -> RED
            if ((resetReas & (1 << 1)) != 0) {
                NRF_LOG_ERROR("Reset Reason - %s", "WATCHDOG");
            }
            // SYSTEM REQUEST -> BLUE
            if ((resetReas & (1 << 2)) != 0) {
                NRF_LOG_INFO("Reset Reason - %s", "SYSTEM REQUEST");
            }
            // LOCKUP -> YELLOW
            if ((resetReas & (1 << 3)) != 0) {
                NRF_LOG_ERROR("Reset Reason - %s", "LOCKUP");
            }
            // Wake from SYSTEM OFF / GPIO -> CYAN
            if ((resetReas & (1 << 16)) != 0) {
                NRF_LOG_INFO("Reset Reason - %s", "WAKE FROM SYSOFF");
            }
            // Debug -> PURPLE
            if ((resetReas & (1 << 18)) != 0) {
                NRF_LOG_INFO("Reset Reason - %s", "DEBUG");
            }
            #endif

            NeopixelDeinit();
        }
    } else {
        NRF_LOG_ERROR("Board not found!");
    }

}


// The board we're currently using
const struct Board_t* getBoard() {
    return currentBoard;
}
