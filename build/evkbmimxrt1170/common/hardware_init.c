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
#include "coex_nb_uart_fw_download.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"



/*${function:start}*/

#if CONFIG_COEX_NB_BSP_TRIGGER
#define COEX_NB_BSP_RTS_GPIO     GPIO11
#define COEX_NB_BSP_RTS_PIN      14U
#define COEX_NB_BSP_RTS_MUX_GPIO IOMUXC_GPIO_DISP_B2_13_GPIO11_IO14
#define COEX_NB_BSP_RTS_MUX_UART IOMUXC_GPIO_DISP_B2_13_LPUART2_RTS_B

int coex_nb_bsp_trigger(void)
{
    /* Toggle BT UART RTS (LPUART2_RTS_B = GPIO_DISP_B2_13 = GPIO11 IO14)
     * low then high to wake BT CPU from boot-sleep.
     */
    gpio_pin_config_t rts_cfg = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};

    IOMUXC_SetPinMux(COEX_NB_BSP_RTS_MUX_GPIO, 0U);
    GPIO_PinInit(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, &rts_cfg);
    GPIO_PinWrite(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, 0U);
    OSA_TimeDelay(5);
    GPIO_PinWrite(COEX_NB_BSP_RTS_GPIO, COEX_NB_BSP_RTS_PIN, 1U);
    IOMUXC_SetPinMux(COEX_NB_BSP_RTS_MUX_UART, 0U);
    (void)PRINTF("BSP trigger sent: RTS toggled to wake BT from boot-sleep\r\n");

    return 0;
}
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */

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
