/* arch/arm/mach-msm/rpc_server_handset.c
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/switch.h>
#include <linux/slab.h>

#include <asm/mach-types.h>

#include <mach/msm_rpcrouter.h>
#include <mach/board.h>
#include <mach/gpio.h>

#define DRIVER_NAME	"msm-handset"

#define HS_SERVER_PROG 0x30000062
#define HS_SERVER_VERS 0x00010001

#define HS_RPC_PROG 0x30000091

#define HS_RPC_VERS_1 0x00010001
#define HS_RPC_VERS_2 0x00020001

#define HS_SUBSCRIBE_SRVC_PROC 0x03
#define HS_REPORT_EVNT_PROC    0x05
#define HS_EVENT_CB_PROC	1
#define HS_EVENT_DATA_VER	1

#define RPC_KEYPAD_NULL_PROC 0
#define RPC_KEYPAD_PASS_KEY_CODE_PROC 2
#define RPC_KEYPAD_SET_PWR_KEY_STATE_PROC 3

#if defined (CONFIG_MACH_EVE)
#define HS_ON_HOOK_K		0x01	/* headphone hook key */
#define GPIO_EAR_SENSE_BIAS		0x1D
#endif
#define HS_PWR_K		0x6F	/* Power key */
#define HS_END_K		0x51	/* End key or Power key */
#define HS_STEREO_HEADSET_K	0x82
#define HS_HEADSET_SWITCH_K	0x84
#define HS_REL_K		0xFF	/* key release */

#define KEY(hs_key, input_key) ((hs_key << 24) | input_key)

enum hs_event {
	HS_EVNT_EXT_PWR = 0,	/* External Power status        */
	HS_EVNT_HSD,		/* Headset Detection            */
	HS_EVNT_HSTD,		/* Headset Type Detection       */
	HS_EVNT_HSSD,		/* Headset Switch Detection     */
	HS_EVNT_KPD,
	HS_EVNT_FLIP,		/* Flip / Clamshell status (open/close) */
	HS_EVNT_CHARGER,	/* Battery is being charged or not */
	HS_EVNT_ENV,		/* Events from runtime environment like DEM */
	HS_EVNT_REM,		/* Events received from HS counterpart on a
				remote processor*/
	HS_EVNT_DIAG,		/* Diag Events  */
	HS_EVNT_LAST,		 /* Should always be the last event type */
	HS_EVNT_MAX		/* Force enum to be an 32-bit number */
};

enum hs_src_state {
	HS_SRC_STATE_UNKWN = 0,
	HS_SRC_STATE_LO,
	HS_SRC_STATE_HI,
};

struct hs_event_data {
	uint32_t	ver;		/* Version number */
	enum hs_event	event_type;     /* Event Type	*/
	enum hs_event	enum_disc;     /* discriminator */
	uint32_t	data_length;	/* length of the next field */
	enum hs_src_state	data;    /* Pointer to data */
	uint32_t	data_size;	/* Elements to be processed in data */
};

enum hs_return_value {
	HS_EKPDLOCKED     = -2,	/* Operation failed because keypad is locked */
	HS_ENOTSUPPORTED  = -1,	/* Functionality not supported */
	HS_FALSE          =  0, /* Inquired condition is not true */
	HS_FAILURE        =  0, /* Requested operation was not successful */
	HS_TRUE           =  1, /* Inquired condition is true */
	HS_SUCCESS        =  1, /* Requested operation was successful */
	HS_MAX_RETURN     =  0x7FFFFFFF/* Force enum to be a 32 bit number */
};

struct hs_key_data {
	uint32_t ver;        /* Version number to track sturcture changes */
	uint32_t code;       /* which key? */
	uint32_t parm;       /* key status. Up/down or pressed/released */
};

enum hs_subs_srvc {
	HS_SUBS_SEND_CMD = 0, /* Subscribe to send commands to HS */
	HS_SUBS_RCV_EVNT,     /* Subscribe to receive Events from HS */
	HS_SUBS_SRVC_MAX
};

enum hs_subs_req {
	HS_SUBS_REGISTER,    /* Subscribe   */
	HS_SUBS_CANCEL,      /* Unsubscribe */
	HS_SUB_STATUS_MAX
};

enum hs_event_class {
	HS_EVNT_CLASS_ALL = 0, /* All HS events */
	HS_EVNT_CLASS_LAST,    /* Should always be the last class type   */
	HS_EVNT_CLASS_MAX
};

enum hs_cmd_class {
	HS_CMD_CLASS_LCD = 0, /* Send LCD related commands              */
	HS_CMD_CLASS_KPD,     /* Send KPD related commands              */
	HS_CMD_CLASS_LAST,    /* Should always be the last class type   */
	HS_CMD_CLASS_MAX
};

/*
 * Receive events or send command
 */
union hs_subs_class {
	enum hs_event_class	evnt;
	enum hs_cmd_class	cmd;
};

struct hs_subs {
	uint32_t                ver;
	enum hs_subs_srvc	srvc;  /* commands or events */
	enum hs_subs_req	req;   /* subscribe or unsubscribe  */
	uint32_t		host_os;
	enum hs_subs_req	disc;  /* discriminator    */
	union hs_subs_class      id;
};

struct hs_event_cb_recv {
	uint32_t cb_id;
	uint32_t hs_key_data_ptr;
	struct hs_key_data key;
};

static const uint32_t hs_key_map[] = {
	KEY(HS_PWR_K, KEY_POWER),
	KEY(HS_END_K, KEY_END),
	KEY(HS_STEREO_HEADSET_K, SW_HEADPHONE_INSERT),
	KEY(HS_HEADSET_SWITCH_K, KEY_MEDIA),
#if defined (CONFIG_MACH_EVE)
	KEY(HS_ON_HOOK_K, KEY_HP),
#endif
	0
};

enum {
	NO_DEVICE	= 0,
	MSM_HEADSET	= 1,
};

static struct input_dev *kpdev;

static int hs_find_key(uint32_t hscode)
{
	int i, key;

	key = KEY(hscode, 0);

	for (i = 0; hs_key_map[i] != 0; i++) {
		if ((hs_key_map[i] & 0xff000000) == key)
			return hs_key_map[i] & 0x00ffffff;
	}
	return -1;
}

/*
 * tuple format: (key_code, key_param)
 *
 * old-architecture:
 * key-press = (key_code, 0)
 * key-release = (0xff, key_code)
 *
 * new-architecutre:
 * key-press = (key_code, 0)
 * key-release = (key_code, 0xff)
 */
static void report_hs_key(uint32_t key_code, uint32_t key_parm)
{
	int key, temp_key_code;

	if (key_code == HS_REL_K)
		key = hs_find_key(key_parm);
	else
		key = hs_find_key(key_code);

	temp_key_code = key_code;

	if (key_parm == HS_REL_K)
		key_code = key_parm;

	switch (key) {
	case KEY_POWER:
	case KEY_END:
		input_report_key(kpdev, key, (key_code != HS_REL_K));
		input_sync(kpdev);
		break;

	case KEY_HP:
		if(gpio_get_value(GPIO_EAR_SENSE_BIAS) == 1)
		{
			if (!kpdev) {
				printk(KERN_ERR "%s: No input device for reporting "
						"pwr/end key press\n", __func__);
				return;
			}
			input_report_key(kpdev, key, (key_code != HS_REL_K));
			input_sync(kpdev);
		}
		break;

	case KEY_MEDIA:
		input_report_key(kpdev, key, (key_code != HS_REL_K));
		break;
	case -1:
		printk(KERN_ERR "%s: No mapping for remote handset event %d\n",
				 __func__, temp_key_code);
		return;
	}
	input_sync(kpdev);
}

static int handle_hs_rpc_call(struct msm_rpc_server *server,
			   struct rpc_request_hdr *req, unsigned len)
{
	struct rpc_keypad_pass_key_code_args {
		uint32_t key_code;
		uint32_t key_parm;
	} *args;

	switch (req->procedure) {
	case RPC_KEYPAD_NULL_PROC:
		return 0;

	case RPC_KEYPAD_PASS_KEY_CODE_PROC: {
		args = (struct rpc_keypad_pass_key_code_args *)(req + 1);
		report_hs_key( be32_to_cpu(args->key_code), be32_to_cpu(args->key_parm) );
		return 0;
	}

	case RPC_KEYPAD_SET_PWR_KEY_STATE_PROC:
		/* This RPC function must be available for the ARM9
		 * to function properly.  This function is redundant
		 * when RPC_KEYPAD_PASS_KEY_CODE_PROC is handled. So
		 * input_report_key is not needed.
		 */
		return 0;
	default:
		printk("%s: unhandled procedure %d\n",__func__,req->procedure);
		return 0;
	}
}

static struct msm_rpc_server hs_rpc_server = {
	.prog		= HS_SERVER_PROG,
	.vers		= HS_SERVER_VERS,
	.rpc_call	= handle_hs_rpc_call,
};

static int __devinit hs_probe(struct platform_device *pdev)
{
	int rc;

	kpdev = input_allocate_device();
	if (!kpdev) {
		rc = -ENOMEM;
		return rc;
	}
	if (pdev->dev.platform_data)
		kpdev->name = pdev->dev.platform_data;
	else
		kpdev->name	= DRIVER_NAME;

	kpdev->id.vendor	= 0x0001;
	kpdev->id.product	= 1;
	kpdev->id.version	= 1;

	input_set_capability(kpdev, EV_KEY, KEY_MEDIA);
	input_set_capability(kpdev, EV_SW, SW_HEADPHONE_INSERT);
	input_set_capability(kpdev, EV_KEY, KEY_POWER);
	input_set_capability(kpdev, EV_KEY, KEY_END);

	rc = input_register_device(kpdev);
	if (rc) {
		dev_err(&kpdev->dev,
				"hs_probe: input_register_device rc=%d\n", rc);
		goto err_reg_input_dev;
	}

	platform_set_drvdata(pdev, kpdev);

	rc = msm_rpc_create_server(&hs_rpc_server);
	if (rc < 0) {
		goto err_hs_rpc_init;
		pr_err("%s: failed to create rpc server\n", __func__);
	}

	return 0;

err_hs_rpc_init:
	input_unregister_device(kpdev);
	kpdev = NULL;
err_reg_input_dev:
	input_free_device(kpdev);
	return rc;
}

static int __devexit hs_remove(struct platform_device *pdev)
{
	struct input_dev *kp = platform_get_drvdata(pdev);

	input_unregister_device(kp);
	return 0;
}

static struct platform_driver hs_driver = {
	.probe		= hs_probe,
	.remove		= __devexit_p(hs_remove),
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init hs_init(void)
{
	return platform_driver_register(&hs_driver);
}
late_initcall(hs_init);

static void __exit hs_exit(void)
{
	platform_driver_unregister(&hs_driver);
}
module_exit(hs_exit);

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:msm-handset");
