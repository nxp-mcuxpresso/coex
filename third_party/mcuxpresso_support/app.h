/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file app.h
 * @brief Application configuration header for coex MCUX builds
 *
 * This file provides basic application-level definitions required by
 * SDK components that reference app.h. It is specifically created for
 * MCUXpresso IDE builds where the ot-nxp repository is not accessible.
 *
 * @note
 * Original: ot-nxp/.../board/rdrw612bga/app.h
 * Referenced by: mcuxsdk/.../low_power/board_lp.c
 * UART macros derived from: ot-nxp/boards/rw612/board.h
 */

#if CONFIG_OT
#ifdef __MCUXPRESSO

#ifndef _APP_H_
#define _APP_H_

#if defined(gAppUseSerialManager_c) && (gAppUseSerialManager_c > 0)
/*
 * UART configuration for serial manager
 * 
 * These macros are derived from ot-nxp/boards/rw612/board.h
 * for BOARD_APP_UART_INSTANCE=0 configuration.
 * 
 * BOARD_APP_UART_INSTANCE must be defined as 0 in project settings
 * to match the OpenThread library build.
 */

/* UART0 configuration - from board.h */
#define BOARD_APP_UART              USART0
#define BOARD_APP_UART_FRG_CLK      \
    (&(const clock_frg_clk_config_t){0, kCLOCK_FrgPllDiv, 255, 0})
#define BOARD_APP_UART_CLK_ATTACH   kFRG_to_FLEXCOMM0

#endif /* defined(gAppUseSerialManager_c) && (gAppUseSerialManager_c > 0) */

/**
 * @brief Initialize board hardware
 *
 * This function initializes the board-level hardware components.
 * It is typically called early in the application startup sequence.
 */
void BOARD_InitHardware(void);

#endif /* _APP_H_ */

#endif /* __MCUXPRESSO */
#endif /* CONFIG_OT */
