/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#ifndef MBEDTLS_CONFIG_CLIENT_H
#define MBEDTLS_CONFIG_CLIENT_H

#if CONFIG_OT_CLI
#include "ot-nxp-mbedtls-config.h"
#include "rt1060-mbedtls-psa-config.h"
#undef PSA_WANT_ALG_JPAKE
#endif

#include "coex_mbedtls_psa_crypto_config.h"

#if (!CONFIG_WPA_SUPP_MBEDTLS) && (!CONFIG_DISABLE_BLE)
#include "mbedtls_user_config.h" //for ble only case
#else
/* Not include mbedtls config file. */
#endif /* CONFIG_WPA_SUPP_MBEDTLS */
#endif /* MBEDTLS_CONFIG_CLIENT_H */