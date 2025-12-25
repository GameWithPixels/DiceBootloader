
#include "svcs_board_config.h"

#if defined(PIXELS_BOOTLOADER)
#include "nrf_svc_function.h"
#else
#if !defined(PIXELS_FIRMWARE_DEBUG)
#include "nrf_sdm.h"
#include "nrf_svc.h"
#include "nrf_svc_function.h"
#include "app_error.h"
SVCALL(PIXELS_SVCS_GETBOARD_ID, uint8_t, svcs_getBoardId_SVC());
#endif
#endif

#if defined(PIXELS_BOOTLOADER) || defined(PIXELS_FIRMWARE_DEBUG)
#include "../board_config.h"
#endif

enum BoardId svcs_getBoardId() {
#if defined(PIXELS_BOOTLOADER)
    return getBoard()->boardId;
#else
    #if defined(PIXELS_FIRMWARE_DEBUG)
        boardInit();
        return getBoard()->boardId;
    #else
        // Set the vector table base address to the bootloader
        ret_code_t err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_ADDRESS);
        APP_ERROR_CHECK(err_code);

        uint8_t ret = svcs_getBoardId_SVC();

        // Set the vector table base address back to main application.
        err_code = sd_softdevice_vector_table_base_set(CODE_START);
        APP_ERROR_CHECK(err_code);
        return ret;
    #endif
#endif
}


#if !defined(PIXELS_BOOTLOADER) && defined(PIXELS_FIRMWARE_DEBUG)
#include "../board_config.c"
#include "../a2d.c"
#include "../neopixel.c"
#endif

#ifdef PIXELS_BOOTLOADER
NRF_SVC_FUNCTION_REGISTER(PIXELS_SVCS_GETBOARD_ID, getBoardId_instance, svcs_getBoardId);
#endif