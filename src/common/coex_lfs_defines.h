/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COEX_LFS_DEFINES_H_
#define _COEX_LFS_DEFINES_H_

#include <stddef.h>
#include "FreeRTOS.h"

/* Redirect lfs_malloc/lfs_free to FreeRTOS heap for coex applications. */
#define LFS_MALLOC(sz) pvPortMalloc(sz)
#define LFS_FREE(p)    vPortFree(p)

#endif /* _COEX_LFS_DEFINES_H_ */
