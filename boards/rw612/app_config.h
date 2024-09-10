/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

/* @TEST_ANCHOR */
#define WIFI_BOARD_RW610
/* @END_TEST_ANCHOR */

#if defined(WIFI_BOARD_RW610)
#define WIFI_BT_USE_IMU_INTERFACE 1
#define RW610

#define CONFIG_MONOLITHIC_WIFI              1
#define CONFIG_MONOLITHIC_BT                0
#define CONFIG_MONOLITHIC_IEEE802154        1
#if ((CONFIG_MONOLITHIC_WIFI) || (CONFIG_MONOLITHIC_BT) || (CONFIG_MONOLITHIC_IEEE802154))
#define CONFIG_MFG_MONOLITHIC               0
#define CONFIG_SOC_SERIES_RW6XX_REVISION_A2 1
#define gPlatformMonolithicApp_d            1
#endif

#define WIFI_BT_TX_PWR_LIMITS "wlan_txpwrlimit_cfg_WW_rw610.h"
#else
#error "Please define macro for RW610 board"
#endif

#include "wifi_config.h"

#if(CONFIG_COEX_SHELL)
#include "app_bluetooth_config.h"
#endif

#ifndef CONFIG_BT_SNOOP
    #define CONFIG_BT_SNOOP 1
#endif
