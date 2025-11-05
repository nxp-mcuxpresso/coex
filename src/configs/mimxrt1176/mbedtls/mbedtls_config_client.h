/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#ifndef MBEDTLS_CONFIG_CLIENT_H
#define MBEDTLS_CONFIG_CLIENT_H

#if CONFIG_WPA_SUPP_MBEDTLS
#include "wpa_supp_mbedtls_config.h"
#else
#if !CONFIG_DISABLE_BLE
#include "mbedtls_user_config.h" //for ble only case
#endif /* !CONFIG_DISABLE_BLE */
#endif /* CONFIG_WPA_SUPP_MBEDTLS */

#endif /* MBEDTLS_CONFIG_CLIENT_H */
