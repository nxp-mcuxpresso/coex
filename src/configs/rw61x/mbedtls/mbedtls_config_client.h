/*
 * Copyright 2022-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

#ifndef MBEDTLS_CONFIG_CLIENT_H
#define MBEDTLS_CONFIG_CLIENT_H

#if CONFIG_OT_CLI
#undef PSA_WANT_ALG_JPAKE
#include "ot-nxp-mbedtls-config.h"
#include "rw612-mbedtls-psa-config.h"
#undef PSA_WANT_ALG_JPAKE
#endif

#include "coex_mbedtls_psa_crypto_config.h"

#ifndef MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#ifdef MBEDTLS_PRIVATE
#undef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(member) member
#endif

#endif /* MBEDTLS_CONFIG_CLIENT_H */
