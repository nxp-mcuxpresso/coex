/*
 * Copyright 2025 NXP
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
#if defined(CONFIG_BT_HOST_CRYPTO) && !defined(CONFIG_BT_CRYPTO_PSA)
#include "ksdk_mbedtls.h"
#endif /* CONFIG_BT_HOST_CRYPTO && !CONFIG_BT_CRYPTO_PSA*/
#include <zephyr/drivers/bluetooth.h>



/*${function:start}*/

void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
#if (defined(WIFI_88W8987_BOARD_MURATA_1ZM_M2) || defined(WIFI_IW416_BOARD_MURATA_1XK_M2) || \
     defined(WIFI_IW612_BOARD_MURATA_2EL_M2) || defined(WIFI_IW610_BOARD_MURATA_2LL_M2))
    BOARD_InitM2UARTPins();
#else
    BOARD_InitArduinoUARTPins();
#endif
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    DMAMUX_Type *dmaMuxBases[] = DMAMUX_BASE_PTRS;
    edma_config_t config;
    DMA_Type *dmaBases[] = DMA_BASE_PTRS;
    DMAMUX_Init(dmaMuxBases[0]);
    EDMA_GetDefaultConfig(&config);
    EDMA_Init(dmaBases[0], &config);
#endif

#if defined(CONFIG_BT_HOST_CRYPTO) && !defined(CONFIG_BT_CRYPTO_PSA)
    CRYPTO_InitHardware();
#endif /* CONFIG_BT_HOST_CRYPTO && !CONFIG_BT_CRYPTO_PSA*/
}

#if (defined(WIFI_88W8987_BOARD_MURATA_1ZM_M2) || defined(WIFI_IW416_BOARD_MURATA_1XK_M2) || \
     defined(WIFI_IW612_BOARD_MURATA_2EL_M2) || defined(WIFI_IW610_BOARD_MURATA_2LL_M2) || defined(WIFI_AW611_BOARD_UBX_JODY_W5_M2))
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc        = CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart2);
#if (defined(WIFI_AW611_BOARD_UBX_JODY_W5_M2))
    config->defaultBaudrate = BOARD_BT_UART_BAUDRATE;
#elif (defined(WIFI_88W8987_BOARD_MURATA_1ZM_M2) || defined(WIFI_IW416_BOARD_MURATA_1XK_M2) || \
       defined(WIFI_IW612_BOARD_MURATA_2EL_M2) || defined(WIFI_IW610_BOARD_MURATA_2LL_M2))
    config->defaultBaudrate = 115200U;
#endif
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance        = 2;
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    config->dma_instance     = 0U;
    config->rx_channel       = 0U;
    config->tx_channel       = 1U;
    config->dma_mux_instance = 0U;
    config->rx_request       = kDmaRequestMuxLPUART2Rx;
    config->tx_request       = kDmaRequestMuxLPUART2Tx;
#endif
    config->enableRxRTS = 1u;
    config->enableTxCTS = 1u;
    return 0;
}
#elif defined(BT_THIRD_PARTY_TRANSCEIVER)
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc         = BOARD_BT_UART_CLK_FREQ;
    config->defaultBaudrate  = 1000000u;
    config->runningBaudrate  = 1000000u;
    config->instance         = BOARD_BT_UART_INSTANCE;
    config->enableRxRTS      = 1u;
    config->enableTxCTS      = 1u;
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    config->dma_instance     = 0U;
    config->rx_channel       = 1U;
    config->tx_channel       = 2U;
    config->dma_mux_instance = 0U;
    config->rx_request       = kDmaRequestMuxLPUART7Rx;
    config->tx_request       = kDmaRequestMuxLPUART7Tx;
#endif
    return 0;
}

void controller_init(void)
{
}

#endif
/*${function:end}*/
