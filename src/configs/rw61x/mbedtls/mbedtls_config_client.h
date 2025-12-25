/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#ifndef MBEDTLS_CONFIG_CLIENT_H
#define MBEDTLS_CONFIG_CLIENT_H

#if CONFIG_OT_CLI
#include "rw612-mbedtls-config.h"
#elif CONFIG_WPA_SUPP_MBEDTLS
#include "wpa_supp_els_pkc_mbedtls_config.h"
#elif !CONFIG_DISABLE_BLE
#include "mbedtls_user_config.h" //for ble only case
#else
/*Not include mbedtls config file.*/
#endif /* CONFIG_OT_CLI */

#ifndef MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#endif /* MBEDTLS_CONFIG_CLIENT_H */
