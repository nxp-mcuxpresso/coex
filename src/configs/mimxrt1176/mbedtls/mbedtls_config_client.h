/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#ifndef MBEDTLS_CONFIG_CLIENT_H
#define MBEDTLS_CONFIG_CLIENT_H

#include "ksdk_mbedtls_config.h"
#undef FREESCALE_PKHA_LONG_OPERANDS_ENABLE
#undef MBEDTLS_FREESCALE_FREERTOS_CALLOC_ALT

#if CONFIG_WPA_SUPP_MBEDTLS
#include "wpa_supp_mbedtls_config.h"
#endif /* CONFIG_WPA_SUPP_MBEDTLS */
#endif /* MBEDTLS_CONFIG_CLIENT_H */
