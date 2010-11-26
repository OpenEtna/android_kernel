/* LGE_CHANGES LGE_FACTORY_AT_COMMANDS  */
/* Created by princlee@lge.com
 * arch/arm/mach-msm/rpc_server_misc.c
 *
 * Copyright (C) 2008 LGE, Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/msm_rpcrouter.h>

#include <linux/syscalls.h>
#include <linux/fcntl.h>	//LGE_CHANGE [seypark@lge.com] for AT+MTC
#include <linux/timer.h>	//LGE_CHANGE [taekeun1.kim@lge.com] 2010-03-07 for kernel timer
/* Misc server definitions. */

#define MISC_APPS_APISPROG		0x30000006
#define MISC_APPS_APISVERS		0

#define ONCRPC_MISC_APPS_APIS_NULL_PROC 	0
#define ONCRPC_LCD_MESSAGE_PROC 		1
#define ONCRPC_LCD_DEBUG_MESSAGE_PROC 		2

#define ONCRPC_LGE_ATCMD_FACTORY_PROC 		3
#define ONCRPC_LGE_ATCMD_MSG_PROC 		4
#define ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC 	5
#define ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC 	6
#define ONCRPC_LGE_GET_FLEX_MCC_PROC 		7	//LGE_UPDATE_S [seypark@lge.com] 2009.07.24 - Get Flex MCC/MNC value
#define ONCRPC_LGE_GET_FLEX_MNC_PROC 		8	//LGE_UPDATE_S [seypark@lge.com] 2009.07.24 - Get Flex MCC/MNC value
#define ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC 	9	//LGE_UPDATE_S [seypark@lge.com] 2009.08.03 - Get Flex Operator ?value
#define ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC 	10	//LGE_UPDATE_S [seypark@lge.com] 2009.08.03 - Get Flex Operator ?value

#define DSATAPIPROG		0x30000074
#define DSATAPIVERS		0

#define ONCRPC_DSATAPI_NULL_PROC 	0
#define ONCRPC_DSAT_CHANGE_BAUD_PROC 	1
#define ONCRPC_DSAT_GET_BAUD_PROC 	2
#define ONCRPC_DSAT_GET_SREG_VAL_PROC 	3
#define ONCRPC_DSAT_AT_CMD_TEST 	4

static int handle_misc_rpc_call(struct msm_rpc_server *server,
				struct rpc_request_hdr *req, unsigned len)
{
	switch (req->procedure) {
	case ONCRPC_MISC_APPS_APIS_NULL_PROC:
		printk(KERN_INFO "ONCRPC_MISC_APPS_APIS_NULL_PROC\n");
		return 0;

	case ONCRPC_LCD_MESSAGE_PROC:
		{
			printk(KERN_INFO "ONCRPC_LCD_MESSAGE_PROC\n");
		}
		return 0;

	case ONCRPC_LCD_DEBUG_MESSAGE_PROC:
		{
			printk(KERN_INFO "ONCRPC_LCD_MESSAGE_PROC\n");
		}

		return 0;
	case ONCRPC_LGE_ATCMD_FACTORY_PROC:
		{
			printk("ONCRPC_LGE_ATCMD_FACTORY_PROC\n");
		}
		return 0;
	case ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC:
		{
			printk("ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC\n");
		}
		return 0;
	case ONCRPC_LGE_ATCMD_MSG_PROC:
		{
			printk(KERN_INFO "ONCRPC_LGE_ATCMD_MSG_PROC\n");
		}
		return 0;

	case ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC:
		{
			printk(KERN_INFO "ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC\n");
		}
		return 0;
	case ONCRPC_LGE_GET_FLEX_MCC_PROC:
		{
			printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_MCC_PROC\n");
			return 0;
		}
	case ONCRPC_LGE_GET_FLEX_MNC_PROC:
		{
			printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_MNC_PROC\n");
			return 0;
		}
	case ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC:
		{
			printk(KERN_INFO
			       "ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC\n");
            return 0;
		}
	case ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC:
		{
			printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_COUNTRY_PROC\n");
			return 0;
		}
	default:
		return -ENODEV;
	}
}

static struct msm_rpc_server rpc_server = {
	.prog = MISC_APPS_APISPROG,
	.vers = MISC_APPS_APISVERS,
	.rpc_call = handle_misc_rpc_call,
};

static int __init rpc_misc_server_init(void)
{
	return msm_rpc_create_server(&rpc_server);
}

module_init(rpc_misc_server_init);
