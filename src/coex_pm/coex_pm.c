/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

 ///////////////////////////////////////////////////////////////////////////////
//  Includes
///////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "fsl_debug_console.h"
#include "coex_pm.h"
#include "PWR_Interface.h"
#include "board_lp.h"
#include "fsl_component_serial_manager.h"

extern void APP_SetTicklessIdle(bool enable);

#define LOWPOWER_DEFAULT_ENABLE_DURATION_MS (10000U)

bool pm_state = false;
static PWR_LowpowerMode_t g_currentMode = PWR_PowerDown;
static TimerHandle_t      lpTimer       = NULL;

static void APP_ConfigureNextLowPowerMode(uint8_t nextMode, uint32_t timeMs)
{
    (void)PWR_ReleaseLowPowerModeConstraint(g_currentMode); /* MISRA CID 26556646 */
    (void)PWR_SetLowPowerModeConstraint((PWR_LowpowerMode_t)nextMode);
    g_currentMode = nextMode;

    if (timeMs > 0U)
    {
        /* Update timer duration */
        (void)xTimerChangePeriod(lpTimer, timeMs / portTICK_PERIOD_MS, 0);

        /* Start the timer, during this time, the configured low power mode will be used as much as possible by
         * the system, when the timer expires, the low power mode will be limited to WFI until next command
         * This is to make sure the serial interface becomes available again to the user */
        if (xTimerStart(lpTimer, 0) != pdPASS)
        {
            assert(0);
        }
    }
}

void pm_cmd(char *command, int argc, char **argv)
{
    uint8_t constraint = 0U;
    uint32_t time_ms = 0xffffffffUL;

    (void)command;

    if (argc > 3)
    {
        PRINTF("COEX PM command: Invaild paramter numbers.\r\n");
        return;
    }

    if (pm_state == false)
    {
        constraint = atoi(argv[1]);
        if (argc == 3)
        {
            time_ms = atoi(argv[2]);
        }

        APP_ConfigureNextLowPowerMode(constraint, time_ms);
        APP_SetTicklessIdle(true);

        pm_state = true;
    }

    return;
}


static void APP_LPTimerCallback(TimerHandle_t timer_h)
{
    (void)timer_h;

    APP_ConfigureNextLowPowerMode(PWR_WFI, 0U);

    APP_SetTicklessIdle(false);
    pm_state = false;
}

void APP_LPTimerCreate(void)
{
    static bool once = false;
    if (!once)
    {
        /* Create FreeRTOS timer which will be used to disable low power after a specific time */
        lpTimer = xTimerCreate("LP timer", (TickType_t)(LOWPOWER_DEFAULT_ENABLE_DURATION_MS / portTICK_PERIOD_MS),
                               pdFALSE, NULL, &APP_LPTimerCallback);
        assert(lpTimer != NULL);
        once = true;
    }
}