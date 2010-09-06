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
#include <mach/rpc_charger.h>
#include <asm/mach-types.h>
/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.03 */
#if defined(CONFIG_MACH_EVE)
#include "proc_comm.h"
//#define DEBUG_CHG

#ifdef DEBUG_CHG
#define D(fmt,args...) printk(fmt, ##args)
#else
#define D(fmt,args...) do { } while(0)
#endif
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.03 */

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
/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-01-20, Add RPC functions in charger rpc */
#if defined(CONFIG_MACH_EVE)
	unsigned long lge_chg_get_sb_info_read;
	unsigned long lge_chg_get_sb_info;
	unsigned long lge_chg_lge_batt_info;
	unsigned long lge_chg_mtoa_init_ok;
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
	unsigned long lge_chg_get_test_sleep_gpio_congif;
	unsigned long lge_chg_set_test_sleep_gpio_congif;
#endif				//#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
	unsigned long lge_chg_pm_test;
#endif
/* LGE_CHANGE_E [jinwooanm@lge.com] 2008-01-20 */
	unsigned chg_read_Battery_voltage_proc;
	unsigned chg_read_Battery_lifepercentRARC_proc;
};

static struct msm_chg_rpc_ids chg_rpc_ids;

static int msm_chg_init_rpc(unsigned long vers)
{
	if (((vers & RPC_VERSION_MAJOR_MASK) == 0x00010000) ||
	    ((vers & RPC_VERSION_MAJOR_MASK) == 0x00020000)) {
		chg_ep = msm_rpc_connect_compatible(MSM_RPC_CHG_PROG, vers,
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
#if defined(CONFIG_MACH_EVE)
		chg_rpc_ids.lge_chg_get_sb_info_read = 12;
		chg_rpc_ids.lge_chg_get_sb_info = 13;
		chg_rpc_ids.lge_chg_lge_batt_info = 14;
		chg_rpc_ids.lge_chg_mtoa_init_ok = 15;
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
		chg_rpc_ids.lge_chg_get_test_sleep_gpio_congif = 16;
		chg_rpc_ids.lge_chg_set_test_sleep_gpio_congif = 17;
#endif //#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
		chg_rpc_ids.lge_chg_pm_test = 18;
#endif
/* LGE_CHANGE_E [jinwoonam@lge.com] 2008-01-20, Add shutdonw test code */

		/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-18, from cupcake */
		chg_rpc_ids.chg_read_Battery_voltage_proc = 21;
		chg_rpc_ids.chg_read_Battery_lifepercentRARC_proc = 22;
		/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-18 */
		return 0;
	} else
		return -ENODATA;
}

/* rpc connect for charging */
int msm_chg_rpc_connect(void)
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
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-07-28, fixed rpc problem */
	//jyoo
#if 0
	chg_ep = msm_rpc_connect_compatible(chg_rpc_ids.prog,
					    chg_rpc_ids.vers_comp, 0);
#endif
	chg_ep = msm_rpc_connect_compatible(chg_rpc_ids.prog, chg_rpc_ids.vers_comp, 0x0001);	//with MSM_RPC_UNINTERRUPTIBLE
	//jyoo end
/* LGE_CHANGE_E [ljmblueday@lge.com] 2009-07-28 */

	printk(KERN_ERR "%s: connect compatible failed \n", __func__);
	return -EAGAIN;

      chg_found:
	printk(KERN_INFO "%s: connected to rpc vers = %x\n",
	       __func__, chg_vers);
	return 0;
}

EXPORT_SYMBOL(msm_chg_rpc_connect);

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

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.03 */
#if defined(CONFIG_MACH_EVE)
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
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.03 */

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

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-07-26, Get batt info */
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

	if (!chg_ep || IS_ERR(chg_ep)) {
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
