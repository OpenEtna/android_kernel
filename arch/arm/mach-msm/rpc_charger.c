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

int msm_chg_get_lge_batt_info(struct lge_battery_info_type* batt_info)
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

#include <mach/msm_rpcrouter.h>
#include <linux/debugfs.h>

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

static int lge_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);
static void lge_battery_external_power_changed(struct power_supply *psy);
static int lge_power_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);

static struct lge_battery_info_type batt_info;

struct lge_battery_device_info {
	struct device *dev;
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

struct lge_battery_device_info *pg_lge_batt_info;

int lge_battery_info_changed(struct lge_battery_info_type *new_batt_info) {

	return (new_batt_info->batt_remaining			!= batt_info.batt_remaining
			|| new_batt_info->batt_voltage		!= batt_info.batt_voltage
			|| new_batt_info->batt_temperature	!= batt_info.batt_temperature
			|| new_batt_info->chg_status		!= batt_info.chg_status
			|| new_batt_info->charger_type		!= batt_info.charger_type
			|| new_batt_info->present			!= batt_info.present);
}

void lge_battery_info_update(struct lge_battery_info_type *new_batt_info) {

	batt_info.batt_remaining	= new_batt_info->batt_remaining;
	batt_info.batt_voltage		= new_batt_info->batt_voltage;
	batt_info.batt_temperature	= new_batt_info->batt_temperature;
	batt_info.chg_status		= new_batt_info->chg_status;
	batt_info.charger_type		= new_batt_info->charger_type;
	batt_info.present			= new_batt_info->present;
}

void notify_usb_connected(int online) {
	cancel_delayed_work(&lge_batt.monitor_work);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ/2);
}
EXPORT_SYMBOL(notify_usb_connected);

static void lge_battery_work(struct work_struct *work) {

	struct lge_battery_info_type new_batt_info;
	int rc;
	const int interval = HZ * 120;
	//const int interval = HZ * 2;

	rc = msm_chg_get_lge_batt_info(&new_batt_info);
	if(rc > 0) {
		printk("%s() %d%%-%dmV-%ddegC-%d(stat)-%d(chg)-%d(pre)\n", __func__, new_batt_info.batt_remaining,
				new_batt_info.batt_voltage, new_batt_info.batt_temperature,
				new_batt_info.chg_status, new_batt_info.charger_type, new_batt_info.present);

		if ( lge_battery_info_changed(&new_batt_info) || new_batt_info.batt_remaining < 4)
		{
			lge_battery_info_update(&new_batt_info);
			power_supply_changed(&lge_power_supplies[BATTERY]);
		}
	} else {
		printk(KERN_ERR "[BAT] lge_battery_read() fail \n");
	}

	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, interval);
}

unsigned int battery_get_current_charge(void)
{
	return batt_info.batt_remaining;
}
EXPORT_SYMBOL(battery_get_current_charge);

static void lge_battery_external_power_changed(struct power_supply *psy)
{
	printk("%s - entered\n", __func__ );
}

static int lge_battery_get_charging_status(void)
{
	int ret = POWER_SUPPLY_STATUS_UNKNOWN;

	lprintk(D_BATT, "%s()\n", __func__);

	switch (batt_info.charger_type) {
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
				val->intval = (batt_info.charger_type == CHG_CHARGER_IRQ__WALL_VALID);
				D("%s() :: POWER_SUPPLY_PROP_ONLINE :: MAINS = %d \n", __func__, val->intval );
				/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06 */
			}
			else if (psy->type == POWER_SUPPLY_TYPE_USB)
			{
				/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
				val->intval = (batt_info.charger_type == CHG_CHARGER_IRQ__USB_VALID);
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

	// Define & Init work queue for battery polling
	INIT_DELAYED_WORK(&lge_batt.monitor_work, lge_battery_work);
	lge_batt.monitor_wqueue = create_singlethread_workqueue("lge_battery_wq");
	if (!lge_batt.monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}

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

	// Battery polling start
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 1);

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

	cancel_rearming_delayed_workqueue(lge_batt.monitor_wqueue,
			&lge_batt.monitor_work);
	destroy_workqueue(lge_batt.monitor_wqueue);

	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		power_supply_unregister(&lge_power_supplies[i]);
	}

	return 0;
}

#ifdef CONFIG_PM
static int lge_battery_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	cancel_delayed_work(&lge_batt.monitor_work);
	return 0;
}

static int lge_battery_resume(struct platform_device *pdev)
{
	lge_battery_work(0);
	return 0;
}

#endif /* CONFIG_PM */

static struct platform_driver lge_battery_driver = {
	.probe = lge_battery_probe,
	.remove   = lge_battery_remove,
#ifdef CONFIG_PM
	.suspend  = lge_battery_suspend,
	.resume	  = lge_battery_resume,
#endif
	.driver = {
		.name = "lge_battery",
		.owner = THIS_MODULE,
	},
};

static int __init lge_battery_init(void)
{
	lprintk(D_BATT, "%s() start !!  \n", __func__ );

	msm_chg_rpc_connect();
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
