/** include/asm-arm/arch-msm/msm_rpcrouter.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2009, Code Aurora Forum. All rights reserved.
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef _AT_KPD_EVE_H
#define _AT_KPD_EVE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/platform_device.h>

/* LGE_CHANGES LGE_FACTORY_AT_COMMANDS  */
//LGE_CHANGE_S [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+MOT,GKPD, FKPD
void vibrator_disable(void);//antispoon@lge.com, diyu@lge.com
void vibrator_enable(void);//antispoon@lge.com, diyu@lge.com
int is_vib_state(void);
void vibrator_set(int value);

int at_gkpd_cfg( unsigned int type, int value );
void write_gkpd_value( int value );
int at_fkpd_cfg ( unsigned int type, int value);
//LGE_CHANGE_E [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+MOT,GKPD, FKPD
/* LGE_CHANGES LGE_FACTORY_AT_COMMANDS  */
#endif
