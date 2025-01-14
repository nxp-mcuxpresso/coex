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
#else
/* SDK mbetdls config include */
#include "els_pkc_mbedtls_config.h"
#ifdef CONFIG_WPA_SUPP_MBEDTLS
#include "wpa_supp_els_pkc_mbedtls_config.h"
#endif /* CONFIG_WPA_SUPP_MBEDTLS */
#endif /* CONFIG_OT_CLI */
#endif /* MBEDTLS_CONFIG_CLIENT_H */
