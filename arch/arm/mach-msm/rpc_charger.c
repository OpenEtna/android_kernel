/* linux/arch/arm/mach-msm/rpc_charger.c
 *
 * Copyright (C) 2010, LGE Inc.
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

#include <linux/err.h>
#include <asm/mach-types.h>
#include <mach/msm_rpcrouter.h>
#include "rpc_charger.h"
#include "proc_comm.h"

//#define DEBUG_CHG

#ifdef DEBUG_CHG
#define D(fmt,args...) printk(fmt, ##args)
#else
#define D(fmt,args...) do { } while(0)
#endif
#define D_FUNC D

#define lprintk(x,args...) D(args)

static struct msm_rpc_endpoint *chg_ep;

#define MSM_RPC_CHG_PROG 0x3000001a

struct msm_chg_rpc_ids {
	unsigned long prog;
	unsigned long vers_comp;
	unsigned chg_is_charging;
	unsigned chg_usb_charger_connected_proc;
	unsigned chg_usb_charger_disconnected_proc;
	unsigned chg_usb_i_is_available_proc;
	unsigned chg_usb_i_is_not_available_proc;
	unsigned long lge_chg_get_sb_info_read;
	unsigned long lge_chg_get_sb_info;
	unsigned long lge_chg_lge_batt_info;
	unsigned long lge_chg_mtoa_init_ok;
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
	unsigned long lge_chg_get_test_sleep_gpio_congif;
	unsigned long lge_chg_set_test_sleep_gpio_congif;
#endif				//#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
	unsigned long lge_chg_pm_test;
	unsigned chg_read_Battery_voltage_proc;
	unsigned chg_read_Battery_lifepercentRARC_proc;
};

static struct msm_chg_rpc_ids chg_rpc_ids;

static int msm_chg_init_rpc(unsigned long vers)
{
	if (((vers & RPC_VERSION_MAJOR_MASK) == 0x00010000) ||
			((vers & RPC_VERSION_MAJOR_MASK) == 0x00020000)) {
		chg_ep = msm_rpc_connect(MSM_RPC_CHG_PROG, vers,
				MSM_RPC_UNINTERRUPTIBLE);
		if (IS_ERR(chg_ep))
			return -ENODATA;

		chg_rpc_ids.vers_comp = vers;
		/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
		chg_rpc_ids.chg_is_charging = 2;
		chg_rpc_ids.chg_usb_charger_connected_proc = 7;
		chg_rpc_ids.chg_usb_charger_disconnected_proc = 8;
		chg_rpc_ids.chg_usb_i_is_available_proc = 9;
		chg_rpc_ids.chg_usb_i_is_not_available_proc = 10;
		/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-01-20, Add RPC functions in charger rpc */
		chg_rpc_ids.lge_chg_get_sb_info_read = 12;
		chg_rpc_ids.lge_chg_get_sb_info = 13;
		chg_rpc_ids.lge_chg_lge_batt_info = 14;
		chg_rpc_ids.lge_chg_mtoa_init_ok = 15;
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
		chg_rpc_ids.lge_chg_get_test_sleep_gpio_congif = 16;
		chg_rpc_ids.lge_chg_set_test_sleep_gpio_congif = 17;
#endif //#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
		chg_rpc_ids.lge_chg_pm_test = 18;

		chg_rpc_ids.chg_read_Battery_voltage_proc = 21;
		chg_rpc_ids.chg_read_Battery_lifepercentRARC_proc = 22;
		return 0;
	} else
		return -ENODATA;
}

/* rpc connect for charging */
static int msm_chg_rpc_connect(void)
{
	uint32_t chg_vers;

	if (machine_is_msm7201a_surf() || machine_is_msm7x27_surf() ||
			machine_is_qsd8x50_surf() || machine_is_msm7x25_surf())
		return -ENOTSUPP;

	if (chg_ep && !IS_ERR(chg_ep)) {
		printk(KERN_INFO "%s: chg_ep already connected\n", __func__);
		return 0;
	}

	chg_vers = 0x00020001;
	if (!msm_chg_init_rpc(chg_vers))
		goto chg_found;

	chg_vers = 0x00010001;
	if (!msm_chg_init_rpc(chg_vers))
		goto chg_found;
	chg_ep = msm_rpc_connect(chg_rpc_ids.prog, chg_rpc_ids.vers_comp, 0x0001);	//with MSM_RPC_UNINTERRUPTIBLE

	printk(KERN_ERR "%s: connect compatible failed \n", __func__);
	return -EAGAIN;

chg_found:
	printk(KERN_INFO "%s: connected to rpc vers = %x\n",
			__func__, chg_vers);
	return 0;
}

int msm_chg_usb_charger_connected(uint32_t device)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t otg_dev;
	} req;

	if (!chg_ep || IS_ERR(chg_ep))
		return -EAGAIN;
	req.otg_dev = cpu_to_be32(device);
	rc = msm_rpc_call(chg_ep, chg_rpc_ids.chg_usb_charger_connected_proc,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: charger_connected failed! rc = %d\n",
				__func__, rc);
	} else
		printk(KERN_INFO "msm_chg_usb_charger_connected\n");

	return rc;
}

EXPORT_SYMBOL(msm_chg_usb_charger_connected);

int msm_chg_usb_i_is_available(uint32_t sample)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t i_ma;
	} req;

	if (!chg_ep || IS_ERR(chg_ep))
		return -EAGAIN;
	req.i_ma = cpu_to_be32(sample);
	rc = msm_rpc_call(chg_ep, chg_rpc_ids.chg_usb_i_is_available_proc,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: charger_i_available failed! rc = %d\n",
				__func__, rc);
	} else
		printk(KERN_INFO "msm_chg_usb_i_is_available\n");

	return rc;
}

EXPORT_SYMBOL(msm_chg_usb_i_is_available);

int msm_chg_usb_i_is_not_available(void)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;

	if (!chg_ep || IS_ERR(chg_ep))
		return -EAGAIN;
	rc = msm_rpc_call(chg_ep, chg_rpc_ids.chg_usb_i_is_not_available_proc,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: charger_i_not_available failed! rc ="
				"%d \n", __func__, rc);
	} else
		printk(KERN_INFO "msm_chg_usb_i_is_not_available\n");

	return rc;
}

EXPORT_SYMBOL(msm_chg_usb_i_is_not_available);

int rpc_chg_is_charging(uint32_t * result)
{
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;
	struct chgrpc_return_data_rsp {
		struct rpc_reply_hdr hdr;
		uint32_t rpc_return;
	} rsp;
	int rc;

	if (!chg_ep || IS_ERR(chg_ep)) {
		return -EAGAIN;
	}

	memset(&rsp, 0, sizeof(rsp));
	rc = msm_rpc_call_reply(chg_ep, chg_rpc_ids.chg_is_charging,
			&req, sizeof(req), &rsp, sizeof(rsp), (5 * HZ));
	if (rc < 0)
		return rc;

	*result = be32_to_cpu(rsp.rpc_return);

	return 0;
}

EXPORT_SYMBOL(rpc_chg_is_charging);

int rpc_read_batt_voltage(uint32_t * result)
{
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;
	struct chgrpc_return_data_rsp {
		struct rpc_reply_hdr hdr;
		uint32_t rpc_return;
	} rsp;
	int rc;

	if (!chg_ep || IS_ERR(chg_ep)) {
		return -EAGAIN;
	}

	memset(&rsp, 0, sizeof(rsp));
	rc = msm_rpc_call_reply(chg_ep,
			chg_rpc_ids.chg_read_Battery_voltage_proc, &req,
			sizeof(req), &rsp, sizeof(rsp), (5 * HZ));
	if (rc < 0)
		return rc;

	*result = be32_to_cpu(rsp.rpc_return);
	D(KERN_INFO "rpc_read_batt_voltage = %d\n", *result);

	return 0;
}

EXPORT_SYMBOL(rpc_read_batt_voltage);

int rpc_read_batt_lifepercentageRARC(uint32_t * result)
{
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;
	struct chgrpc_return_data_rsp {
		struct rpc_reply_hdr hdr;
		uint32_t rpc_return;
	} rsp;
	int rc;

	if (!chg_ep || IS_ERR(chg_ep)) {
		return -EAGAIN;
	}

	memset(&rsp, 0, sizeof(rsp));
	rc = msm_rpc_call_reply(chg_ep,
			chg_rpc_ids.
			chg_read_Battery_lifepercentRARC_proc, &req,
			sizeof(req), &rsp, sizeof(rsp), (5 * HZ));
	if (rc < 0)
		return rc;

	*result = be32_to_cpu(rsp.rpc_return);
	D(KERN_INFO "rpc_read_batt_lifepercentageRARC = %d%%, %d\n", *result,
			rsp.rpc_return);

	return 0;
}

EXPORT_SYMBOL(rpc_read_batt_lifepercentageRARC);

int msm_chg_usb_charger_disconnected(void)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;

	if (!chg_ep || IS_ERR(chg_ep))
		return -EAGAIN;
	rc = msm_rpc_call(chg_ep, chg_rpc_ids.chg_usb_charger_disconnected_proc,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: charger_disconnected failed! rc = %d\n",
				__func__, rc);
	} else
		printk(KERN_INFO "msm_chg_usb_charger_disconnected\n");

	return rc;
}

EXPORT_SYMBOL(msm_chg_usb_charger_disconnected);

int msm_chg_get_lge_batt_info(lge_battery_info_type * batt_info)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
	} req;
	struct hsusb_rpc_rep {
		struct rpc_reply_hdr hdr;
		uint32_t batt_remaining;
		uint32_t batt_voltage;
		uint32_t batt_temperature;
		uint32_t chg_status;
		uint32_t charger_type;
		uint32_t present;
	} rep;

	if (!chg_ep)
		msm_chg_rpc_connect();

	if (IS_ERR(chg_ep)) {
		lprintk(D_BATT, "%s: msm_chg_lge_batt_info rpc failed before"
				"call, rc = %ld\n", __func__, PTR_ERR(chg_ep));
		return -EAGAIN;
	}

	rc = msm_rpc_call_reply(chg_ep, chg_rpc_ids.lge_chg_get_sb_info,
			&req, sizeof(req), &rep, sizeof(rep), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: msm_chg_lge_batt_info failed! rc = %d\n",
				__func__, rc);
	} else {
		batt_info->batt_remaining = be32_to_cpu(rep.batt_remaining);
		batt_info->batt_voltage = be32_to_cpu(rep.batt_voltage);
		//batt_info->batt_current = be32_to_cpu(rep.batt_current);
		batt_info->batt_temperature = be32_to_cpu(rep.batt_temperature) * 10;	// Android uses 0.1C drgree
		//batt_info->batt_flag = be32_to_cpu(rep.batt_flag);
		batt_info->chg_status = be32_to_cpu(rep.chg_status);
		batt_info->charger_type = be32_to_cpu(rep.charger_type);
		batt_info->present = be32_to_cpu(rep.present);
	}

	return rc;
}

EXPORT_SYMBOL(msm_chg_get_lge_batt_info);
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009-07-26 */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-03-02, checking if MtoA is initialized */
int msm_chg_mtoa_init_ok(int mtoa_init_ok)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t mtoa_init_ok;
	} req;
	struct hsusb_rpc_rep {
		struct rpc_reply_hdr hdr;
	} rep;

	lprintk(D_BATT, "%s() start !!	\n", __func__);

	if (!chg_ep || IS_ERR(chg_ep)) {
		lprintk(D_BATT, "%s: msm_chg_mtoa_init_ok rpc failed before"
				"call, rc = %ld\n", __func__, PTR_ERR(chg_ep));
		return -EAGAIN;
	}

	rc = msm_rpc_call_reply(chg_ep, chg_rpc_ids.lge_chg_mtoa_init_ok,
			&req, sizeof(req), &rep, sizeof(rep), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR "%s: msm_chg_mtoa_init_ok failed! rc = %d\n",
				__func__, rc);
	}

	return rc;
}

EXPORT_SYMBOL(msm_chg_mtoa_init_ok);
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009-03-02 */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-03-19, Add test mode for changing GPIO setting during sleep */
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
int msm_chg_lge_get_test_sleep_gpio_config(int gpio_number)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t gpio_num;
	} req;
	struct hsusb_rpc_rep {
		struct rpc_reply_hdr hdr;
		int32_t result;
	} rep;

	req.gpio_num = cpu_to_be32((uint32_t) (gpio_number));

	lprintk(D_BATT, "%s() start !!	\n", __func__);

	if (!chg_ep || IS_ERR(chg_ep)) {
		lprintk(D_BATT,
				"%s: msm_chg_lge_get_test_sleep_gpio_config rpc failed before"
				"call, rc = %ld\n", __func__, PTR_ERR(chg_ep));
		return -EAGAIN;
	}

	rc = msm_rpc_call_reply(chg_ep,
			chg_rpc_ids.lge_chg_get_test_sleep_gpio_congif,
			&req, sizeof(req), &rep, sizeof(rep), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR
				"%s: msm_chg_lge_get_test_sleep_gpio_config failed! rc = %d\n",
				__func__, rc);
	} else {
		lprintk(D_BATT,
				" %s : gpio# = %d, returned sleep config value = %d \n",
				__func__, gpio_number, be32_to_cpu(rep.result));
	}

	return be32_to_cpu(rep.result);
}

EXPORT_SYMBOL(msm_chg_lge_get_test_sleep_gpio_config);

int msm_chg_lge_set_test_sleep_gpio_config(int gpio_number, int gpio_config)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t gpio_num;
		uint32_t gpio_conf;
	} req;
	struct hsusb_rpc_rep {
		struct rpc_reply_hdr hdr;
	} rep;

	req.gpio_num = cpu_to_be32((uint32_t) (gpio_number));
	req.gpio_conf = cpu_to_be32((uint32_t) (gpio_config));

	lprintk(D_BATT, "%s() start !!	\n", __func__);

	if (!chg_ep || IS_ERR(chg_ep)) {
		lprintk(D_BATT,
				"%s: msm_chg_lge_set_test_sleep_gpio_config rpc failed before"
				"call, rc = %ld\n", __func__, PTR_ERR(chg_ep));
		return -EAGAIN;
	}

	rc = msm_rpc_call_reply(chg_ep,
			chg_rpc_ids.lge_chg_set_test_sleep_gpio_congif,
			&req, sizeof(req), &rep, sizeof(rep), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR
				"%s: msm_chg_lge_get_test_sleep_gpio_config failed! rc = %d\n",
				__func__, rc);
	}

	return rc;
}

EXPORT_SYMBOL(msm_chg_lge_set_test_sleep_gpio_config);

#endif //#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009-03-19 */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2008-04-01, Add shutdonw test code */
int msm_chg_pm_test(int test_order)
{
	int rc = 0;
	int test_flag = 0x1234;

	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t lge_test_order;
	} req;

	if (test_order < TEST_RPC_MAX)	// call rpc
	{
		req.lge_test_order = cpu_to_be32((uint32_t) (test_order));

		if (!chg_ep || IS_ERR(chg_ep)) {
			printk(KERN_ERR
					"%s: msm_chg_go_shutdown rpc failed before"
					"call, rc = %ld\n", __func__, PTR_ERR(chg_ep));
			return -EAGAIN;
		}
		rc = msm_rpc_call(chg_ep, chg_rpc_ids.lge_chg_pm_test,
				&req, sizeof(req), 5 * HZ);

		if (rc < 0) {
			printk(KERN_ERR
					"%s: charger_i_not_available failed! rc ="
					"%d \n", __func__, rc);
		} else
			printk(KERN_INFO "msm_chg_usb_i_is_not_available\n");
	} else {
		switch (test_order) {
			case TEST_SMEM_SHUT_DOWN:
				msm_proc_comm(PCOM_POWER_DOWN, 0, 0);
				for (;;) ;
				break;
			case TEST_SMEM:
				msm_proc_comm(PCOM_CUSTOMER_CMD1, &test_flag, 0);
				break;
		}
	}

	return rc;
}

EXPORT_SYMBOL(msm_chg_pm_test);
/* LGE_CHANGE_E [jinwoonam@lge.com] 2008-04-01 */

/* rpc call to close charging connection */
int msm_chg_rpc_close(void)
{
	int rc = 0;

	if (IS_ERR(chg_ep)) {
		printk(KERN_ERR "%s: rpc_close failed before call, rc = %ld\n",
				__func__, PTR_ERR(chg_ep));
		return -EAGAIN;
	}

	rc = msm_rpc_close(chg_ep);
	chg_ep = NULL;

	if (rc < 0) {
		printk(KERN_ERR "%s: close rpc failed! rc = %d\n",
				__func__, rc);
		return -EAGAIN;
	} else
		printk(KERN_INFO "rpc close success\n");

	return rc;
}

EXPORT_SYMBOL(msm_chg_rpc_close);
/*
 * Driver for batteries of lge.
 *
 */

#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/miscdevice.h>

// Select Test Mode interface
#define SYSFS_LGE_BATT_TEST_MODE
#define DEBUGFS_LGE_BATT_TEST_MODE

// Select RPC method
//#define LGE_BATT_MTOA_RPC_MODE
#define LGE_BATT_ATOM_RPC_MODE

/* LGE_CHANGE_S [ybw75@lge.com] 2009-01-21, Add Charger MtoA (modem to app) RPC routine */
#include <mach/msm_rpcrouter.h>
/* LGE_CHANGE_E [ybw75@lge.com] 2009-01-21 */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-02-04, Add Debugfs feature for test mode */
#include <linux/debugfs.h>
/* LGE_CHANGE_E [ybw75@lge.com] 2009-02-04  */

#define LGE_BATT_DEFAULT_VOLTAGE_MV		3800		// mV
#define LGE_BATT_DEFAULT_CAPACITY		50		// %
#define LGE_BATT_DEFAULT_TEMPERATURE		300		// 0.1 Celsius degree

enum charger_type {
	CHG_HOST_PC,
	CHG_WALL = 2,
	CHG_UNDEFINED,
};


int msm_chg_pm_test( int test_order);

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
#ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
extern int get_charger_type( void );
#endif
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06  */

enum {
	BEFORE_INITIALIZATION = 0,
	AFTER_INITIALIZATION = 1,
};

static int lge_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);
static void lge_battery_external_power_changed(struct power_supply *psy);
static int lge_power_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);

static lge_battery_info_type new_batt_info, batt_info;

//static smart_battery_info_type sbat_info;

int work_queue_init_ok=BEFORE_INITIALIZATION;

struct lge_battery_device_info {
	struct device *dev;

	unsigned long update_time;	/* jiffies when data read */
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

static struct lge_battery_device_info lge_batt;

enum lge_power_type {
	BATTERY=0,
	USB = 1,
	AC = 2,
};
static char *supply_list[] = {
	"battery",
};
static enum power_supply_property lge_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
};

static enum power_supply_property lge_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
static struct power_supply lge_power_supplies[] = {
	{	/* battery */
		.name = "battery",
		.type	   = POWER_SUPPLY_TYPE_BATTERY,
		.properties	   = lge_battery_props,
		.num_properties = ARRAY_SIZE(lge_battery_props),
		.get_property   = lge_battery_get_property,
		.external_power_changed = lge_battery_external_power_changed,
	},
	{	/* USB charger */
		.name	   = "usb",
		.type	   = POWER_SUPPLY_TYPE_USB,
		.properties	   = lge_power_props,
		.num_properties = ARRAY_SIZE(lge_power_props),
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.get_property   = lge_power_get_property,
		.external_power_changed = NULL,
	},
	{	/* AC charger */
		.name    = "ac",
		.type    = POWER_SUPPLY_TYPE_MAINS,
		.properties	  = lge_power_props,
		.num_properties = ARRAY_SIZE(lge_power_props),
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.get_property   = lge_power_get_property,
		.external_power_changed = NULL,
	},
};

#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
enum {
	BATT_TEST_MODE = 0,
	BATT_TEST_CAPACITY,
	BATT_TEST_VOLT,
	BATT_TEST_TEMP,
	BATT_TEST_PM,
};
enum {
	LGE_BATT_TEST_MODE_OFF = 0,
	LGE_BATT_TEST_MODE_ON = 1,
};
int	lge_battery_test_mode=LGE_BATT_TEST_MODE_OFF;
#endif

struct lge_battery_device_info *pg_lge_batt_info;

#if defined(SYSFS_LGE_BATT_TEST_MODE)
static int lge_battery_test_create_attrs(struct device * dev);
static ssize_t lge_battery_test_show_property(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t lge_battery_test_store_property(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
#endif
#if defined(DEBUGFS_LGE_BATT_TEST_MODE)
void lge_battery_debug_init(void);
#endif

static unsigned int cache_time = 1000;
module_param(cache_time, uint, 0644);
MODULE_PARM_DESC(cache_time, "cache time in milliseconds");


int lge_battery_info_changed(void)
{
	if ( new_batt_info.batt_remaining 		!= batt_info.batt_remaining
			|| new_batt_info.batt_voltage	!= batt_info.batt_voltage
			|| new_batt_info.batt_temperature!= batt_info.batt_temperature
			|| new_batt_info.chg_status		!= batt_info.chg_status
			|| new_batt_info.charger_type	!= batt_info.charger_type
			|| new_batt_info.present		!= batt_info.present)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void lge_battery_info_update(void)
{
	batt_info.batt_remaining 	= new_batt_info.batt_remaining;
	batt_info.batt_voltage		= new_batt_info.batt_voltage;
	batt_info.batt_temperature	= new_batt_info.batt_temperature;
	batt_info.chg_status		= new_batt_info.chg_status;
	batt_info.charger_type		= new_batt_info.charger_type;
	batt_info.present			= new_batt_info.present;
}

static int lge_battery_read(void)
{
	int rc=0;

	// If TestMode is ON, Don't update battery info.
#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
	if (LGE_BATT_TEST_MODE_ON == lge_battery_test_mode )
	{
		return 0;
	}
#endif

#if defined(LGE_BATT_ATOM_RPC_MODE)
	rc = msm_chg_get_lge_batt_info(&new_batt_info);
	if (rc>0)
	{
		/*printk("%s() %d%%-%dmV-%ddegC-%d(stat)-%d(chg)-%d(pre)\n", __func__, new_batt_info.batt_remaining, 	\
		  new_batt_info.batt_voltage, new_batt_info.batt_temperature, \
		  new_batt_info.chg_status, new_batt_info.charger_type, new_batt_info.present);
		  */

		if ( lge_battery_info_changed() || new_batt_info.batt_remaining < 4)
		{
			lge_battery_info_update();
			power_supply_changed(&lge_power_supplies[BATTERY]);
		}
	}
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	return rc;
}

static int lge_battery_read_status(void)
{
	int ret=0;

	/* if (not first time) && ( too short time passed after reading data) then return */
	if (lge_batt.update_time && time_before(jiffies, lge_batt.update_time +
				msecs_to_jiffies(cache_time)))
		return 0;

	ret = lge_battery_read();
	if (ret < 0) {
		printk(KERN_ERR "[BAT] lge_battery_read() fail \n");
		return 1;
	}

	lge_batt.update_time = jiffies;

	return 0;
}

#if defined(LGE_BATT_ATOM_RPC_MODE)
void lge_battery_update_charger(void)
{
	D_FUNC("%s()\n", __func__);

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	//queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work,  HZ * 30);
#ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
	if (CHG_UNDEFINED == get_charger_type())

		new_batt_info.charger_type = CHG_CHARGER_IRQ__WALL_INVALID;
	power_supply_changed(&lge_power_supplies[BATTERY]);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 30);
}
else
#endif
{
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 1);
}
#endif
}

static void lge_battery_work(struct work_struct *work)
{
	//const int interval = HZ * 60;
	const int interval = HZ * 30;

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

	lge_battery_read_status();

#if defined(LGE_BATT_ATOM_RPC_MODE)
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, interval);
#endif
}
#endif //#if defined(LGE_BATT_ATOM_RPC_MODE)

static void lge_battery_external_power_changed(struct power_supply *psy)
{
	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ/10);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

}

static int lge_battery_get_charging_status(void)
{
	int ret = POWER_SUPPLY_STATUS_UNKNOWN;

	lprintk(D_BATT, "%s()\n", __func__);

	// To Do ::
	switch (new_batt_info.charger_type) {
		case CHG_CHARGER_IRQ__WALL_INVALID:
		case CHG_CHARGER_IRQ__USB_INVALID:
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case CHG_CHARGER_IRQ__WALL_VALID:
		case CHG_CHARGER_IRQ__USB_VALID:
			if (100 == batt_info.batt_remaining)
				ret = POWER_SUPPLY_STATUS_FULL;
			else
				ret = POWER_SUPPLY_STATUS_CHARGING;
			break;
		default:
			ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return ret;
}

static int lge_battery_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	D_FUNC("%s() power_supply_property=%d	\n", __func__, psp );

	//lge_battery_read_status();		// Take too long time So use buffered data

	switch (psp) {
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = batt_info.batt_voltage;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = batt_info.batt_remaining;
			break;
			//case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
			//	val->intval = di->rated_capacity;
			//	break;
			//case POWER_SUPPLY_PROP_CHARGE_FULL:
			//	val->intval = di->full_active_uAh;
			//	break;
			//case POWER_SUPPLY_PROP_CHARGE_EMPTY:
			//	val->intval = di->empty_uAh;
			//	break;
			//case POWER_SUPPLY_PROP_CHARGE_NOW:
			//	val->intval = di->accum_current_uAh;
			//	break;
		case POWER_SUPPLY_PROP_TEMP:
			val->intval = batt_info.batt_temperature;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = lge_battery_get_charging_status();
			lprintk(D_BATT, "%s() :: power_supply_property->POWER_SUPPLY_PROP_STATUS :: val->intval = %d	\n", __func__, val->intval );
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = batt_info.present;
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static int lge_power_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	D_FUNC("%s() power_supply_property=%d	\n", __func__, psp );

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			if (psy->type == POWER_SUPPLY_TYPE_MAINS)
			{
				/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
				// To Do ::
				if ( new_batt_info.charger_type == CHG_CHARGER_IRQ__WALL_VALID )
				{
					val->intval = 1;
				}
				else
				{
					val->intval = 0;
				}
				D("%s() :: POWER_SUPPLY_PROP_ONLINE :: MAINS = %d \n", __func__, val->intval );
				/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06 */
			}
			else if (psy->type == POWER_SUPPLY_TYPE_USB)
			{
				/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
				if ( new_batt_info.charger_type == CHG_CHARGER_IRQ__USB_VALID )
				{
					val->intval = 1;
				}
				else
				{
					val->intval = 0;
				}
				D("%s() :: POWER_SUPPLY_PROP_ONLINE :: USB = %d \n", __func__, val->intval );
				/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06  */
			}
			else
			{
				val->intval = 0;
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int lge_battery_probe(struct platform_device *pdev)
{
	int i, retval = 0;

	lprintk(D_BATT, "%s() start !!  \n", __func__ );

	lge_batt.dev = &pdev->dev;

#if defined(LGE_BATT_ATOM_RPC_MODE)
	// Define & Init work queue for battery polling
	INIT_DELAYED_WORK(&lge_batt.monitor_work, lge_battery_work);
	lge_batt.monitor_wqueue = create_singlethread_workqueue("lge_battery_wq");
	if (!lge_batt.monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	// Initialize Battery property with default values
	batt_info.batt_voltage = LGE_BATT_DEFAULT_VOLTAGE_MV;
	batt_info.batt_remaining = LGE_BATT_DEFAULT_CAPACITY;
	batt_info.batt_temperature = LGE_BATT_DEFAULT_TEMPERATURE;

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++)
	{
		retval = power_supply_register(&pdev->dev, &lge_power_supplies[i]);
		if (retval)
		{
			printk("Failed to register power supply (%d)\n", retval);
			goto batt_failed;
		}
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	// Battery polling start
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 1);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

#if defined(SYSFS_LGE_BATT_TEST_MODE)
	lge_battery_test_create_attrs(lge_batt.dev);
#endif
#if defined(DEBUGFS_LGE_BATT_TEST_MODE)
	lge_battery_debug_init();
#endif

	work_queue_init_ok = AFTER_INITIALIZATION;

	goto success;

batt_failed:
	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		power_supply_unregister(&lge_power_supplies[i]);
	}
workqueue_failed:
	destroy_workqueue(lge_batt.monitor_wqueue);
success:
	return retval;
}

static int lge_battery_remove(struct platform_device *pdev)
{
	int i;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_rearming_delayed_workqueue(lge_batt.monitor_wqueue,
			&lge_batt.monitor_work);
	destroy_workqueue(lge_batt.monitor_wqueue);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		power_supply_unregister(&lge_power_supplies[i]);
	}

	return 0;
}

#ifdef CONFIG_PM

static int lge_battery_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	return 0;
}

static int lge_battery_resume(struct platform_device *pdev)
{

	lprintk(D_BATT, "%s() start !!	\n", __func__ );
	D_FUNC("%s()\n", __func__ );

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return 1;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	return 0;
}

#else

#endif /* CONFIG_PM */

#if defined(SYSFS_LGE_BATT_TEST_MODE)

#define LGE_BATTERY_TEST_ATTR(_name)							\
{										\
	.attr = { .name = #_name, .mode = (S_IRUGO|S_IWUGO), .owner = THIS_MODULE },	\
	.show = lge_battery_test_show_property,					\
	.store = lge_battery_test_store_property,								\
}

static struct device_attribute lge_battery_test_attrs[] = {
	LGE_BATTERY_TEST_ATTR(TestMode),
	LGE_BATTERY_TEST_ATTR(TestCapacity),
	LGE_BATTERY_TEST_ATTR(TestVoltage),
	LGE_BATTERY_TEST_ATTR(TestTemperature),
};

static int lge_battery_test_create_attrs(struct device * dev)
{
	int i, rc;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	for (i = 0; i < ARRAY_SIZE(lge_battery_test_attrs); i++) {
		rc = device_create_file(dev, &lge_battery_test_attrs[i]);
		if (rc)
		{
			while (i--)
				device_remove_file(dev, &lge_battery_test_attrs[i]);
		}
	}

	return rc;
}

static ssize_t lge_battery_test_show_property(struct device *p_dev,
		struct device_attribute *attr,
		char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - lge_battery_test_attrs;

	switch (off) {
		case BATT_TEST_MODE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", lge_battery_test_mode);
			lprintk(D_BATT, " BATT_TEST_MODE writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_CAPACITY:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_remaining);
			lprintk(D_BATT, " BATT_TEST_CAPACITY writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_VOLT:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_voltage*1000);
			lprintk(D_BATT, " BATT_TEST_VOLT writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_TEMP:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_temperature);
			lprintk(D_BATT, " BATT_TEST_TEMP writing, success_number = %d	\n", i );
			break;
		default:
			i = -EINVAL;
	}
	return i;
}

static ssize_t lge_battery_test_store_property(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long x;
	const ptrdiff_t off = attr - lge_battery_test_attrs;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	switch (off)  {
		case BATT_TEST_MODE:
			if (sscanf(buf, "%ld\n", &x) == 1)
			{
				lge_battery_test_mode = x ;
				lprintk(D_BATT, " BATT_TEST_MODE reading = %d	\n", (int)x );
			}
			break;
		case BATT_TEST_CAPACITY:
			if (sscanf(buf, "%ld\n", &x) == 1)
			{
				lprintk(D_BATT, " BATT_TEST_CAPACITY reading = %d	\n", (int)x );
				batt_info.batt_remaining = x;
			}
			break;
		case BATT_TEST_VOLT:
			if (sscanf(buf, "%ld\n", &x) == 1)
			{
				lprintk(D_BATT, " BATT_TEST_VOLT reading = %d	\n", (int)x );
				batt_info.batt_voltage = (unsigned int)(x/1000);
			}

			break;
		case BATT_TEST_TEMP:
			if (sscanf(buf, "%ld\n", &x) == 1)
			{
				lprintk(D_BATT, " BATT_TEST_TEMP reading = %d	\n", (int)x );
				batt_info.batt_temperature = x;
			}
			break;
		default:
			count = -EINVAL;
	}

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	return count;
}

#endif	// #if defined(SYSFS_LGE_BATT_TEST_MODE)

#if defined(DEBUGFS_LGE_BATT_TEST_MODE)

struct lge_batt_test_reg {
	const char *name;
	unsigned int code;
};

#define LGE_BATT_TEST_REG(_name, _code) { .name = _name, .code = _code}

static struct lge_batt_test_reg lge_batt_test_regs[] = {
	LGE_BATT_TEST_REG("TestMode",  	 BATT_TEST_MODE),
	LGE_BATT_TEST_REG("TestCapacity",    BATT_TEST_CAPACITY),
	LGE_BATT_TEST_REG("TestVoltage",    BATT_TEST_VOLT),
	LGE_BATT_TEST_REG("TestTemperature", BATT_TEST_TEMP),
	LGE_BATT_TEST_REG("PmTest", BATT_TEST_PM),
};

static int lge_battery_debug_set(void *data, u64 val)
{
	struct lge_batt_test_reg *p_lge_batt_test_reg = data;
	int rtn = 0;

	lprintk(D_BATT, "%s() : p_lge_batt_test_reg->code = %d	\n", __func__ , p_lge_batt_test_reg->code );

	switch (p_lge_batt_test_reg->code)
	{
		case BATT_TEST_MODE:
			lge_battery_test_mode = (int)val;
			break;
		case BATT_TEST_CAPACITY:
			batt_info.batt_remaining = (unsigned int)val;
			break;
		case BATT_TEST_VOLT:
			batt_info.batt_voltage = (unsigned int)((unsigned int)val/1000);
			break;
		case BATT_TEST_TEMP :
			batt_info.batt_temperature = (int)val;
			break;
		case BATT_TEST_PM:
			msm_chg_pm_test( (int)val );
			break;
		default:
			rtn = -EINVAL;
			break;
	}

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	return rtn;

}

static int lge_battery_debug_get(void *data, u64 *val)
{
	struct lge_batt_test_reg *p_lge_batt_test_reg = data;
	int rtn = 0;

	lprintk(D_BATT, "%s() : p_lge_batt_test_reg->code = %d	\n", __func__ , p_lge_batt_test_reg->code );
	switch (p_lge_batt_test_reg->code)
	{
		case BATT_TEST_MODE:
			*val = (u64)lge_battery_test_mode;
			break;
		case BATT_TEST_CAPACITY:
			*val = (u64)(batt_info.batt_remaining);
			break;
		case BATT_TEST_VOLT:
			*val = (u64)(batt_info.batt_voltage*1000);
			break;
		case BATT_TEST_TEMP:
			*val = (u64)(batt_info.batt_temperature);
			break;
		case BATT_TEST_PM:
			*val = 0;
			break;
		default:
			rtn = -EINVAL;
			break;
	}

	return rtn;
}

DEFINE_SIMPLE_ATTRIBUTE(lge_battery_test_fops, lge_battery_debug_get, lge_battery_debug_set, "%llu\n");

void lge_battery_debug_init(void)
{
	struct dentry *dent;
	int n;

	dent = debugfs_create_dir("lge_battery_test", 0);
	if (IS_ERR(dent))
		lprintk(D_BATT, "%s() : debugfs_create_dir() Error !!!! 	\n", __func__ );

	for (n = 0; n < ARRAY_SIZE(lge_batt_test_regs); n++)
	{
		debugfs_create_file(lge_batt_test_regs[n].name, 0666, dent, \
				&lge_batt_test_regs[n],	&lge_battery_test_fops);
	}
}

#endif // #if defined(DEBUGFS_LGE_BATT_TEST_MODE)

static struct platform_driver lge_battery_driver = {
	.probe = lge_battery_probe,
	.remove   = lge_battery_remove,
	.suspend  = lge_battery_suspend,
	.resume	  = lge_battery_resume,
	.driver = {
		.name = "lge_battery",
		.owner = THIS_MODULE,
	},
};

static int __init lge_battery_init(void)
{
	lprintk(D_BATT, "%s() start !!  \n", __func__ );

	return platform_driver_register(&lge_battery_driver);
}

static void __exit lge_battery_exit(void)
{
	platform_driver_unregister(&lge_battery_driver);
}

module_init(lge_battery_init);
module_exit(lge_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Szabolcs Gyurko <szabolcs.gyurko@tlt.hu>, "
		"Matt Reimer <mreimer@vpop.net>, "
		"Anton Vorontsov <cbou@mail.ru>");
MODULE_DESCRIPTION("lge_battery battery driver");
