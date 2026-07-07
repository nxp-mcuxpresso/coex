/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <porting.h>
#include <zephyr/shell/shell.h>

#include "fsl_debug_console.h"

#if !defined(RW612_SERIES)
#include "db_gen.h"
#include "host_msd_fatfs.h"
#endif

#include "coex_shell.h"
#if(CONFIG_WIFI_BLE_COEX_APP)
#include "cli.h"
#endif

extern serial_handle_t g_serialHandle;
struct shell *ctx_shell;

SDK_ALIGN(static uint8_t shell_handle_buffer[SHELL_HANDLE_SIZE], 4);
static shell_handle_t shell_handle;
static struct shell coex_sh;

#if(CONFIG_WIFI_BLE_COEX_APP)
static int cmd_wifi(const struct shell *sh, size_t argc, char **argv)
{
    const struct cli_command *command = NULL;

    if (argc < 2)
    {
        shell_print(sh, "Usage: wifi <wlan-command> [args...]");
        help_command(0, NULL);
        return 0;
    }

    /* Skip argv[0]("wifi"), pass argv+1 to WiFi CLI lookup */
    command = lookup_command(argv[1], strlen(argv[1]));
    if (command != NULL)
    {
        command->function(argc - 1, &argv[1]);
    }
    else
    {
        shell_error(sh, "Unknown wifi command: %s", argv[1]);
    }

    return 0;
}

SHELL_CMD_ARG_REGISTER(wifi, NULL, "WiFi shell commands", cmd_wifi, 1, SHELL_MAX_ARGS);
#endif

#if APP_LOWPOWER_ENABLED
extern void APP_ConfigureNextLowPowerMode(uint8_t nextMode, uint32_t timeMs);
bool pm_state = false;
static int cmd_pm_enable(const struct shell *sh, size_t argc, char *argv[])
{
    uint8_t constraint = 0U;
    uint32_t time_ms = 0xffffffffUL;

    if (argc < 2 || argc > 3)
        return -EINVAL;

    if (pm_state)
    {
        shell_error(sh, "Low power mode already enabled");
        return -EBUSY;
    }

    constraint = (uint8_t)atoi(argv[1]);
    if (constraint < 1 || constraint > 3)
    {
        shell_error(sh, "Invalid constraint value. Must be 1-3");
        pm_state = false;
        return -EINVAL;
    }

    if (argc == 3)
    {
        time_ms = (uint32_t)atoi(argv[2]);
    }

    if (constraint == 1)
    {
        constraint = 0;
    }

    shell_print(sh, "Constraint is PM%d. Ready to enter low power loop for %u ms", constraint, time_ms);
    APP_ConfigureNextLowPowerMode(constraint, time_ms);
    pm_state = true;

    return 0;
}

static int cmd_pm(const struct shell *sh, size_t argc, char **argv)
{
    if (argc == 1) {
        shell_help(sh);
        return SHELL_CMD_HELP_PRINTED;
    }

    shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

    return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pm_cmds,
    SHELL_CMD_ARG(enable, NULL, "<constraint: 1-PM1, 2-PM2, 3-PM3> [time (ms)]", cmd_pm_enable, 2, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pm, &pm_cmds, "Power Manager shell commands", cmd_pm);
#endif

static void printSeparator(void)
{
    PRINTF("========================================\r\n");
}

int coex_cli_init(void)
{
    shell_handle = (shell_handle_t)&shell_handle_buffer[0];
    if (kStatus_SHELL_Success != SHELL_Init(shell_handle, g_serialHandle, CONFIG_SHELL_PROMPT))
    {
        (void)PRINTF("Coex shell initialization failed!\r\n");
        return -1;
    }
    printSeparator();

    ctx_shell = &coex_sh;
    ctx_shell->sh = shell_handle;

    return 0;
}
