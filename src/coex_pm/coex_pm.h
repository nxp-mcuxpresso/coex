/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html
 */

///////////////////////////////////////////////////////////////////////////////
//  Includes
///////////////////////////////////////////////////////////////////////////////

#ifndef __COEX_PM_H__
#define __COEX_PM_H__

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/
void pm_cmd(char *command, int argc, char **argv);
void APP_LPTimerCreate(void);

#endif