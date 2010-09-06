/* arch/arm/mach-msm/proc_comm_batt.h
 *
 * Copyright (C) 2010 LGE, Inc.
 * Author: DJ Kim <dojip.kim@lge.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_MSM_PROC_COMM_BATT_H
#define __MACH_MSM_PROC_COMM_BATT_H

#if defined(CONFIG_MACH_EVE)
extern int proc_comm_batt_is_charging(void);
extern int proc_comm_batt_get_power_on_status(void);
extern int proc_comm_batt_get_voltage(void);
extern int proc_comm_batt_get_level(void);
extern int proc_comm_batt_get_temperature(void);
extern int proc_comm_batt_is_valid(void);
extern int proc_comm_batt_is_charger_connected(void);
extern int proc_comm_batt_get_cable_status(void);
extern int proc_comm_batt_power_down(void);
#else
inline int  proc_comm_batt_is_charging(void)
{
	return 0;
}

inline int proc_comm_batt_get_power_on_status(void)
{
	return 0;
}

inline int proc_comm_batt_get_voltage(void)
{
	return 0;
}

inline int proc_comm_batt_get_level(void)
{
	return 0;
}

inline int proc_comm_batt_get_temperature(void)
{
	return 0;
}

inline int proc_comm_batt_is_valid(void)
{
	return 0;
}

inline int proc_comm_batt_is_charger_connected(void);
{
	return 0;
}

inline int proc_comm_batt_get_cable_status(void);
{
	return 0;
}

inline int proc_comm_batt_power_down(void);
{
	return 0;
}
#endif

#endif
