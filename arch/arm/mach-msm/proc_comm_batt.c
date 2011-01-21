/* arch/arm/mach-msm/proc_comm_batt.c
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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>

#include "proc_comm.h"

#if defined(CONFIG_MACH_EVE)

int proc_comm_batt_is_charging(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_IS_CHARGING;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_is_charging);

int proc_comm_batt_get_power_on_status(void)
{
	int data1, data2;
	if (msm_proc_comm(PCOM_GET_POWER_ON_STATUS, &data1, &data2))
		return -1;

	return data1;
}
EXPORT_SYMBOL(proc_comm_batt_get_power_on_status);

int proc_comm_batt_get_voltage(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_GET_VOLTAGE;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_get_voltage);

int proc_comm_batt_get_level(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_GET_LEVEL;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_get_level);

int proc_comm_batt_get_temperature(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_GET_TEMPERTURE;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_get_temperature);

int proc_comm_batt_is_valid(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_IS_VALID;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_is_valid);

int proc_comm_batt_is_charger_connected(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_IS_CHG_CONNECTED;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_is_charger_connected);

int proc_comm_batt_get_cable_status(void)
{
	int data = 0;
	int fntype = CUSTOMER_CMD2_BATT_GET_CABLE_STATUS;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return data;
}
EXPORT_SYMBOL(proc_comm_batt_get_cable_status);

int proc_comm_batt_power_down(void)
{
	int data;
	int fntype = CUSTOMER_CMD2_BATT_POWER_DOWN;
	if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &data, &fntype))
		return -1;

	return 0;
}
EXPORT_SYMBOL(proc_comm_batt_power_down);
#endif
