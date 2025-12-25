/**
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "nrf_bootloader.h"

#include "compiler_abstraction.h"
#include "nrf.h"
#include "boards.h"
#include "sdk_config.h"
#include "nrf_power.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_dfu.h"
#include "nrf_gpio.h"
#include "nrf_error.h"
#include "nrf_dfu_settings.h"
#include "nrf_dfu_utils.h"
#include "nrf_bootloader_wdt.h"
#include "nrf_bootloader_info.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_fw_activation.h"
#include "nrf_bootloader_dfu_timers.h"
#include "app_scheduler.h"
#include "nrf_dfu_validation.h"
#include "neopixel.h"

static volatile bool m_flash_write_done;

#define SCHED_QUEUE_SIZE      32          /**< Maximum number of events in the scheduler queue. */
#define SCHED_EVENT_DATA_SIZE NRF_DFU_SCHED_EVENT_DATA_SIZE /**< Maximum app_scheduler event size. */

#if !(defined(NRF_BL_DFU_ENTER_METHOD_BUTTON)    && \
      defined(NRF_BL_DFU_ENTER_METHOD_PINRESET)  && \
      defined(NRF_BL_DFU_ENTER_METHOD_GPREGRET)  && \
      defined(NRF_BL_DFU_ENTER_METHOD_BUTTONLESS)&& \
      defined(NRF_BL_RESET_DELAY_MS)             && \
      defined(NRF_BL_DEBUG_PORT_DISABLE))
    #error Configuration file is missing flags. Update sdk_config.h.
#endif

STATIC_ASSERT((NRF_BL_DFU_INACTIVITY_TIMEOUT_MS >= 100) || (NRF_BL_DFU_INACTIVITY_TIMEOUT_MS == 0),
             "NRF_BL_DFU_INACTIVITY_TIMEOUT_MS must be 100 ms or more, or 0 to indicate that it is disabled.");

#if defined(NRF_LOG_BACKEND_FLASH_START_PAGE)
STATIC_ASSERT(NRF_LOG_BACKEND_FLASH_START_PAGE != 0,
    "If nrf_log flash backend is used it cannot use space after code because it would collide with settings page.");
#endif

#define TO_COLOR(red, green, blue) ((uint32_t)((red) << 16) | (uint32_t)((green) << 8) | (uint32_t)(blue))
#define BL_LED_INTENSITY 8

static void flash_write_callback(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    m_flash_write_done = true;
}


static void do_reset(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    NRF_LOG_FINAL_FLUSH();

    nrf_delay_ms(NRF_BL_RESET_DELAY_MS);

    NVIC_SystemReset();
}

ret_code_t custom_bootloader_start_app();
ret_code_t custom_bootloader_enter_dfu();

static void bootloader_reset()
{
    NRF_LOG_DEBUG("Resetting bootloader.");

    // if (do_backup)
    // {
    //     m_flash_write_done = false;
    //     nrf_dfu_settings_backup(do_reset);
    // }
    // else
    {
        do_reset(NULL);
    }
}


static void inactivity_timeout(void)
{
    NRF_LOG_INFO("Inactivity timeout.");
    bootloader_reset();
}

static void custom_app_start(void * p_event_data, uint16_t event_size)
{
    nrf_bootloader_app_start();
}

static int app_start_timeout_count = 0;
static void start_appstart_timeout(void)
{
    app_start_timeout_count = 0;
}

static void UpdateLEDs_AppTimeout() {

    static const uint32_t rainbowColors[] = {
        0x000100,
        0x010200,
        0x030200,
        0x050100,
        0x070000,
        0x060001,
        0x050002,
        0x030004,
        0x020005,
        0x010006,
        0x000007,
        0x000105,
        0x000203,
        0x000201,
    };

    int index = app_start_timeout_count % (sizeof(rainbowColors) / sizeof(rainbowColors[0]));
    uint32_t color = rainbowColors[index];
    SetHighestLED(color);
}

static void UpdateLEDs_BluetoothActivity() {
    static bool flip = false;
    if (flip) {
        SetHighestLED(TO_COLOR(0, 0, BL_LED_INTENSITY));
    } else {
        SetHighestLED(TO_COLOR(0, 0, BL_LED_INTENSITY / 4));
    }
    flip = !flip;
}

static void UpdateLEDs_NoValidApp() {
    // Set highest LED yellow
    SetHighestLED(TO_COLOR(BL_LED_INTENSITY, BL_LED_INTENSITY, 0));
}

static void countdown_app_timeout(void)
{
    app_start_timeout_count++;
    if (app_start_timeout_count == CUSTOM_BL_DFU_INACTIVITY_TIMEOUT_STEPS) {
        // Start the app
        NRF_LOG_INFO("DFU not connected to, starting app now.");
        NRF_LOG_PROCESS();
        app_sched_event_put(NULL, 0, custom_app_start);
    } else {
        UpdateLEDs_AppTimeout();
        nrf_bootloader_dfu_inactivity_timer_restart(NRF_BOOTLOADER_MS_TO_TICKS(CUSTOM_BL_DFU_INACTIVITY_TIMEOUT_MS / CUSTOM_BL_DFU_INACTIVITY_TIMEOUT_STEPS), countdown_app_timeout);
    }
}

/**@brief Function for handling DFU events. */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
        case NRF_DFU_EVT_DFU_STARTED:
        case NRF_DFU_EVT_OBJECT_RECEIVED:
            UpdateLEDs_BluetoothActivity();
            nrf_bootloader_dfu_inactivity_timer_restart(NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS), inactivity_timeout);
            break;
        case NRF_DFU_EVT_DFU_COMPLETED:
        case NRF_DFU_EVT_DFU_ABORTED:
            bootloader_reset();
            break;
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
            // Reset the internal state of the DFU settings to the last stored state.
            UpdateLEDs_NoValidApp();
            nrf_dfu_settings_reinit();
            break;
        default:
            break;
    }
}


/**@brief Function for initializing the event scheduler.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Suspend the CPU until an interrupt occurs.
 */
static void wait_for_event(void)
{
#if defined(BLE_STACK_SUPPORT_REQD) || defined(ANT_STACK_SUPPORT_REQD)
    (void)sd_app_evt_wait();
#else
    // Wait for an event.
    __WFE();
    // Clear the internal event register.
    __SEV();
    __WFE();
#endif
}


/**@brief Continually sleep and process tasks whenever woken.
 */
static void loop_forever(void)
{
    while (true)
    {
        //feed the watchdog if enabled.
        nrf_bootloader_wdt_feed();

        app_sched_execute();

        if (!NRF_LOG_PROCESS())
        {
            wait_for_event();
        }
    }
}

#if NRF_BL_DFU_ENTER_METHOD_BUTTON
#ifndef BUTTON_PULL
    #error NRF_BL_DFU_ENTER_METHOD_BUTTON is enabled but not buttons seem to be available on the board.
#endif
/**@brief Function for initializing button used to enter DFU mode.
 */
static void dfu_enter_button_init(void)
{
    nrf_gpio_cfg_sense_input(NRF_BL_DFU_ENTER_METHOD_BUTTON_PIN,
                             BUTTON_PULL,
                             NRF_GPIO_PIN_SENSE_LOW);
}
#endif


static bool crc_on_valid_app_required(void)
{
    return true;
    // bool ret = true;
    // if (NRF_BL_APP_CRC_CHECK_SKIPPED_ON_SYSTEMOFF_RESET &&
    //     (nrf_power_resetreas_get() & NRF_POWER_RESETREAS_OFF_MASK))
    // {
    //     nrf_power_resetreas_clear(NRF_POWER_RESETREAS_OFF_MASK);
    //     ret = false;
    // }
    // else if (NRF_BL_APP_CRC_CHECK_SKIPPED_ON_GPREGRET2 &&
    //         ((nrf_power_gpregret2_get() & BOOTLOADER_DFU_GPREGRET2_MASK) == BOOTLOADER_DFU_GPREGRET2)
    //         && (nrf_power_gpregret2_get() & BOOTLOADER_DFU_SKIP_CRC_BIT_MASK))
    // {
    //     nrf_power_gpregret2_set(nrf_power_gpregret2_get() & ~BOOTLOADER_DFU_SKIP_CRC);
    //     ret = false;
    // }
    // else
    // {
    // }

    // return ret;
}



static bool boot_validate(boot_validation_t const * p_validation, uint32_t data_addr, uint32_t data_len, bool do_crc)
{
    if (!do_crc && (p_validation->type == VALIDATE_CRC))
    {
        return true;
    }
    return nrf_dfu_validation_boot_validate(p_validation, data_addr, data_len);
}


/** @brief Function for checking if the main application is valid.
 *
 * @details     This function checks if there is a valid application
 *              located at Bank 0.
 *
 * @param[in]   do_crc Perform CRC check on application. Only CRC checks
                       can be skipped. For other boot validation types,
                       this parameter is ignored.
 *
 * @retval  true  If a valid application has been detected.
 * @retval  false If there is no valid application.
 */
static bool app_is_valid(bool do_crc)
{
    if (s_dfu_settings.bank_0.bank_code != NRF_DFU_BANK_VALID_APP)
    {
        NRF_LOG_INFO("Boot validation failed. No valid app to boot.");
        return false;
    }
    else if (NRF_BL_APP_SIGNATURE_CHECK_REQUIRED &&
        (s_dfu_settings.boot_validation_app.type != VALIDATE_ECDSA_P256_SHA256))
    {
        NRF_LOG_WARNING("Boot validation failed. The boot validation of the app must be a signature check.");
        return false;
    }
    else if (SD_PRESENT && !boot_validate(&s_dfu_settings.boot_validation_softdevice, MBR_SIZE, s_dfu_settings.sd_size, do_crc))
    {
        NRF_LOG_WARNING("Boot validation failed. SoftDevice is present but invalid.");
        return false;
    }
    else if (!boot_validate(&s_dfu_settings.boot_validation_app, nrf_dfu_bank0_start_addr(), s_dfu_settings.bank_0.image_size, do_crc))
    {
        NRF_LOG_WARNING("Boot validation failed. App is invalid.");
        return false;
    }
    // The bootloader itself is not checked, since a self-check of this kind gives little to no benefit
    // compared to the cost incurred on each bootup.

    NRF_LOG_DEBUG("App is valid");
    return true;
}



/**@brief Function for clearing all DFU enter flags that
 *        preserve state during reset.
 *
 * @details This is used to make sure that each of these flags
 *          is checked only once after reset.
 */
static void dfu_enter_flags_clear(void)
{
    if (NRF_BL_DFU_ENTER_METHOD_PINRESET &&
       (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk))
    {
        // Clear RESETPIN flag.
        NRF_POWER->RESETREAS |= POWER_RESETREAS_RESETPIN_Msk;
    }

    if (NRF_BL_DFU_ENTER_METHOD_GPREGRET &&
       ((nrf_power_gpregret_get() & BOOTLOADER_DFU_GPREGRET_MASK) == BOOTLOADER_DFU_GPREGRET)
            && (nrf_power_gpregret_get() & BOOTLOADER_DFU_START_BIT_MASK))
    {
        // Clear DFU mark in GPREGRET register.
        nrf_power_gpregret_set(nrf_power_gpregret_get() & ~BOOTLOADER_DFU_START);
    }

    if (NRF_BL_DFU_ENTER_METHOD_BUTTONLESS &&
       (s_dfu_settings.enter_buttonless_dfu == 1))
    {
        // Clear DFU flag in flash settings.
        s_dfu_settings.enter_buttonless_dfu = 0;
        APP_ERROR_CHECK(nrf_dfu_settings_write(NULL));
    }
}


/**@brief Function for checking whether to enter DFU mode or not.
 */
static bool dfu_enter_check(void)
{
    if (!app_is_valid(crc_on_valid_app_required()))
    {
        NRF_LOG_DEBUG("DFU mode because app is not valid.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_GPREGRET &&
       (nrf_power_gpregret_get() & BOOTLOADER_DFU_START))
    {
        NRF_LOG_DEBUG("DFU mode requested via GPREGRET.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_BUTTONLESS &&
       (s_dfu_settings.enter_buttonless_dfu == 1))
    {
        NRF_LOG_DEBUG("DFU mode requested via bootloader settings.");
        return true;
    }

    return false;
}

uint32_t getDeviceID() {
    return NRF_FICR->DEVICEID[1] ^ NRF_FICR->DEVICEID[0];
}

char g_advertised_name[12] = "PXL";

ret_code_t custom_bootloader_enter_dfu() {
    nrf_bootloader_wdt_init();
    scheduler_init();
    dfu_enter_flags_clear();

    const uint32_t uniqueId = getDeviceID();
    for (int i = 0; i < 8; ++i) {
        const char value = (uniqueId >> ((7 - i) << 2)) & 0xf;
        g_advertised_name[i + 3] = value + (value < 10 ? '0' : 'A' - 10);
    }
    g_advertised_name[11] = '\0';

    ret_code_t ret_val = nrf_dfu_init(dfu_observer);
    if (ret_val != NRF_SUCCESS)
    {
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_DEBUG("Enter main loop");
    loop_forever(); // This function will never return.
    NRF_LOG_ERROR("Unreachable");
    return NRF_ERROR_INTERNAL;
}


ret_code_t custom_bootloader_app_valid() {

    m_flash_write_done = false;
    nrf_dfu_settings_backup(flash_write_callback);
    ASSERT(m_flash_write_done);
        
    return NRF_SUCCESS;
}

#if NRF_BL_DFU_ALLOW_UPDATE_FROM_APP
static void postvalidate(void)
{
    NRF_LOG_INFO("Postvalidating update after reset.");
    nrf_dfu_validation_init();

    if (nrf_dfu_validation_init_cmd_present())
    {
        uint32_t firmware_start_addr;
        uint32_t firmware_size;

        // Execute a previously received init packed. Subsequent executes will have no effect.
        if (nrf_dfu_validation_init_cmd_execute(&firmware_start_addr, &firmware_size) == NRF_DFU_RES_CODE_SUCCESS)
        {
            if (nrf_dfu_validation_prevalidate() == NRF_DFU_RES_CODE_SUCCESS)
            {
                if (nrf_dfu_validation_activation_prepare(firmware_start_addr, firmware_size) == NRF_DFU_RES_CODE_SUCCESS)
                {
                    NRF_LOG_INFO("Postvalidation successful.");
                }
            }
        }
    }

    s_dfu_settings.bank_current = NRF_DFU_CURRENT_BANK_0;
    UNUSED_RETURN_VALUE(nrf_dfu_settings_write_and_backup(flash_write_callback));
}
#endif


ret_code_t nrf_bootloader_init(nrf_dfu_observer_t observer)
{
    NRF_LOG_DEBUG("In nrf_bootloader_init");

    ret_code_t                            ret_val;
    nrf_bootloader_fw_activation_result_t activation_result;


    if (NRF_BL_DEBUG_PORT_DISABLE)
    {
        nrf_bootloader_debug_port_disable();
    }

#if NRF_BL_DFU_ENTER_METHOD_BUTTON
    dfu_enter_button_init();
#endif

    ret_val = nrf_dfu_settings_init(false);
    if (ret_val != NRF_SUCCESS)
    {
        return NRF_ERROR_INTERNAL;
    }

    #if NRF_BL_DFU_ALLOW_UPDATE_FROM_APP
    // Postvalidate if DFU has signaled that update is ready.
    if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
    {
        postvalidate();
    }
    #endif

    // Check if an update needs to be activated and activate it.
    activation_result = nrf_bootloader_fw_activate();

    switch (activation_result)
    {
        case ACTIVATION_NONE:
            if (dfu_enter_check()) {
                // DFU is necessary, enter it
                NRF_LOG_INFO("Entering DFU because no valid app");
                UpdateLEDs_NoValidApp();
                nrf_bootloader_dfu_inactivity_timer_restart(NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS), inactivity_timeout);
            } else {
                // App is valid, enter dfu only for a short amount of time, then start the app!
                NRF_LOG_INFO("Entering DFU for a few seconds, because app is valid");
                start_appstart_timeout();
                nrf_bootloader_dfu_inactivity_timer_restart(NRF_BOOTLOADER_MS_TO_TICKS(CUSTOM_BL_DFU_INACTIVITY_TIMEOUT_MS / CUSTOM_BL_DFU_INACTIVITY_TIMEOUT_STEPS), countdown_app_timeout);
                //custom_bootloader_app_valid();
            }
            custom_bootloader_enter_dfu();
            break;

        case ACTIVATION_SUCCESS_EXPECT_ADDITIONAL_UPDATE:
            custom_bootloader_enter_dfu();
            break;

        case ACTIVATION_SUCCESS:
            bootloader_reset();
            NRF_LOG_ERROR("Unreachable");
            return NRF_ERROR_INTERNAL; // Should not reach this.

        case ACTIVATION_ERROR:
        default:
            return NRF_ERROR_INTERNAL;
    }

    // Should not be reached.
    return NRF_ERROR_INTERNAL;
}
