/*
 * Copyright 2023-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*${header:start}*/
#include "pin_mux.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_adapter_uart.h"

#include <zephyr/drivers/bluetooth.h>
#include "fsl_lpuart_edma.h"
#include "fsl_dmamux.h"
#if CONFIG_COEX_NB_BSP_TRIGGER
#include "fsl_iomuxc.h"
#include "fsl_gpio.h"
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */
/*${header:end}*/

/*${macro:start}*/
#if CONFIG_COEX_NB_BSP_TRIGGER
/* LPUART3 RTS_B: GPIO_AD_B1_05 -> GPIO1_IO21 */
#define COEX_NB_BSP_RTS_GPIO     GPIO1
#define COEX_NB_BSP_RTS_PIN      21U
#define COEX_NB_BSP_RTS_MUX_GPIO IOMUXC_GPIO_AD_B1_05_GPIO1_IO21
#define COEX_NB_BSP_RTS_MUX_UART IOMUXC_GPIO_AD_B1_05_LPUART3_RTS_B
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */
/*${macro:end}*/

/*${variable:start}*/

/*${variable:end}*/

/*${function:start}*/

void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
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
}

#if (defined(WIFI_88W8987_BOARD_MURATA_1ZM_M2) || defined(WIFI_IW416_BOARD_MURATA_1XK_M2) || \
     defined(WIFI_IW612_BOARD_MURATA_2EL_M2) || defined(WIFI_IW610_BOARD_MURATA_2LL_M2))
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc        = BOARD_BT_UART_CLK_FREQ;
    config->defaultBaudrate = 115200u;
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance        = BOARD_BT_UART_INSTANCE;
#if (defined(HAL_UART_DMA_ENABLE) && (HAL_UART_DMA_ENABLE > 0U))
    config->dma_instance     = 0U;
    config->rx_channel       = 0U;
    config->tx_channel       = 1U;
    config->dma_mux_instance = 0U;
    config->rx_request       = kDmaRequestMuxLPUART3Rx;
    config->tx_request       = kDmaRequestMuxLPUART3Tx;
#endif
    config->enableRxRTS = 1u;
    config->enableTxCTS = 1u;
    return 0;
}
#endif

#if CONFIG_COEX_NB_BSP_TRIGGER
int coex_nb_bsp_trigger(void)
{
    gpio_pin_config_t rts_cfg = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    IOMUXC_SetPinMux(COEX_NB_BSP_RTS_MUX_GPIO, 0U);
    GPIO_PinInit(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, &rts_cfg);
    GPIO_PinWrite(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, 0U);  /* assert RTS low */
    OSA_TimeDelay(5);
    GPIO_PinWrite(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, 1U);  /* deassert RTS high */
    IOMUXC_SetPinMux(COEX_NB_BSP_RTS_MUX_UART, 0U);
    return 0;
}
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */
/*${function:end}*/
