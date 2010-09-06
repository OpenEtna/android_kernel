/* linux/include/mach/rpc_charger.h
 *
 * Copyright (C) 2010 LGE Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#ifndef __ASM_ARCH_MSM_RPC_CHARGER_H
#define __ASM_ARCH_MSM_RPC_CHARGER_H

#include <mach/msm_rpcrouter.h>
/* LGE_CHANGE_S [ybw75@lge.com] 2009-01-06*/
#if defined(CONFIG_MACH_EVE)
#include <mach/lprintk.h>
#include <mach/charger.h>
#endif
/* LGE_CHANGE_E [ybw75@lge.com] 2009-01-06 */

int msm_chg_rpc_connect(void);
int msm_chg_usb_charger_connected(uint32_t type);
int msm_chg_usb_i_is_available(uint32_t sample);
int msm_chg_usb_i_is_not_available(void);
int msm_chg_usb_charger_disconnected(void);
int msm_chg_rpc_close(void);

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.03 */
#if defined(CONFIG_MACH_EVE)
int rpc_chg_is_charging(uint32_t *result);
int rpc_read_batt_voltage(uint32_t *result);
int rpc_read_batt_lifepercentageRARC(uint32_t *result);
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.03 */
#endif /* CONFIG_MACH_EVE */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-01-06 */
#if defined(CONFIG_MACH_EVE)
//int msm_chg_lge_get_sb_info(smart_battery_info_type* sb_info);
//int msm_chg_lge_batt_info(lge_battery_info_parameter sb_info);
int msm_chg_get_lge_batt_info(lge_battery_info_type* batt_info);
int msm_chg_mtoa_init_ok( int mtoa_init_ok);
#endif
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009-01-06 */

#endif
