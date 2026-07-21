/*
 * Copyright 2022-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include "clock_config.h"
#include "board.h"
#include "pin_mux.h"

#include "fsl_debug_console.h"
#include "fsl_adapter_uart.h"
#include "fsl_lpuart_edma.h"
#include "fsl_dmamux.h"
#include <zephyr/drivers/bluetooth.h>
/*${header:end}*/

/*${macro:start}*/

/* DMA */
#define EXAMPLE_DMAMUX_INSTANCE      (0U)
#define EXAMPLE_DMA_INSTANCE         (0U)

/*${macro:end}*/

/*${variable:start}*/

/*${variable:end}*/

/*${function:start}*/
void BOARD_InitHardware(void)
{
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    DMAMUX_Type *dmaMuxBases[] = DMAMUX_BASE_PTRS;
    edma_config_t config;
    DMA_Type *dmaBases[] = DMA_BASE_PTRS;
#endif

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitSDHCPins();   /* USDHC1 pins for SDIO to IW612 (MAYA-W276) - required for WLAN */
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    DMAMUX_Init(dmaMuxBases[EXAMPLE_DMAMUX_INSTANCE]);
    EDMA_GetDefaultConfig(&config);
    EDMA_Init(dmaBases[EXAMPLE_DMA_INSTANCE], &config);
#endif
}

#if defined(WIFI_IW612_BOARD_MURATA_2EL_M2) || defined(WIFI_IW612_BOARD_UBX_MAYA_W276)
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc        = CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart8);
    config->defaultBaudrate = 115200u;
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance        = 8U;
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    config->dma_instance     = 0U;
    config->rx_channel       = 4U;
    config->tx_channel       = 5U;
    config->dma_mux_instance = 0U;
    config->rx_request       = kDmaRequestMuxLPUART8Rx;
    config->tx_request       = kDmaRequestMuxLPUART8Tx;
#endif
    config->enableRxRTS = 1u;
    config->enableTxCTS = 1u;
    return 0;
}
#endif
/*${function:end}*/
