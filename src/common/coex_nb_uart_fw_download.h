/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COEX_NB_UART_FW_DOWNLOAD_H_
#define _COEX_NB_UART_FW_DOWNLOAD_H_

#include <stdint.h>

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Download NB (BT/OT) firmware over UART.
 *
 * @param fw        Pointer to firmware binary data.
 * @param fw_length Length of firmware binary in bytes.
 *
 * @return 0 on success, negative error code on failure.
 */
int coex_nb_uart_fw_download(const uint8_t *fw, uint32_t fw_length);

#if CONFIG_COEX_NB_BSP_TRIGGER
/*!
 * @brief Board-level BSP trigger to wake narrow band CPU from boot-sleep.
 *
 * Override this weak function to toggle the UART RTS pin low then high,
 * waking the narrow band CPU from boot-sleep in coex scenario.
 *
 * @return 0 on success, negative error code on failure.
 */
int coex_nb_bsp_trigger(void);
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */

#endif /* _COEX_NB_UART_FW_DOWNLOAD_H_ */
