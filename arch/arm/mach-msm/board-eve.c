/* linux/arch/arm/mach-msm/board-eve.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * Copyright (c) 2008-2009 QUALCOMM USA, INC.
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/android_pmem.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/power_supply.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_hsusb.h>
#include <mach/vreg.h>
#include <mach/msm_rpcrouter.h>
#include <mach/memory.h>
#include <mach/camera.h>
#include <mach/msm_battery.h>
#ifdef CONFIG_USB_FUNCTION
#include <linux/usb/mass_storage_function.h>
#endif
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#include <mach/rpc_hsusb.h>
#endif

#include "devices.h"
#include "socinfo.h"
#include <linux/switch.h>
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-26, board-eve-keypad.h instead of msm-keypad-devices.h */
//#include "msm-keypad-devices.h"
#include "board-eve-keypad.h"
#include "pm.h"
#include <linux/eve_at.h> //vlc
#include "proc_comm.h"  // LGE_CHANGE [jinwoonam@lge.com] for proc_comm
#include <mach/system.h>

#ifdef CONFIG_MSM_STACKED_MEMORY
#define MSM_SMI_BASE		0x100000
#define MSM_SMI_SIZE		0x800000

#define MSM_PMEM_GPU0_BASE	MSM_SMI_BASE
#define MSM_PMEM_GPU0_SIZE	0x800000
#endif

#define MSM_PMEM_MDP_SIZE	0x800000
#define MSM_PMEM_CAMERA_SIZE	0xa00000
//#define MSM_PMEM_ADSP_SIZE	0xf00000
#define MSM_PMEM_ADSP_SIZE	0x800000
#define MSM_PMEM_GPU1_SIZE	0x800000
#define MSM_FB_SIZE		0x800000

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define MSM_EBI2_BASE			0x10000000
#define EVE_RAM_CONSOLE_BASE	(MSM_EBI2_BASE + 223 * SZ_1M)
#define	EVE_RAM_CONSOLE_SIZE	(128 * SZ_1K)
#endif

/* for MMC detect */
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
#define GPIO_MMC_CD_N 30
#endif

/* for Handset */
static struct platform_device pwrkey_device = {
	.name   = "msm-handset",
	.id     = -1,
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-08-02, reduce number of mass storage from two to one*/
#ifdef LGE_USB_DRIVER
	.nluns          = 0x01,
#else	
	.nluns          = 0x02,
#endif	
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-08-02 */
	.buf_size       = 16384,
	.vendor         = "LGE",
	.product        = "Android Phone",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0xF000,
	.adb_product_id	= 0x9015,
	.version	= 0x0100,
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
	.nluns = 1,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

#ifdef CONFIG_USB_FUNCTION
static void hsusb_gpio_init(void)
{
	if (gpio_request(111, "ulpi_data_0"))
		pr_err("failed to request gpio ulpi_data_0\n");
	if (gpio_request(112, "ulpi_data_1"))
		pr_err("failed to request gpio ulpi_data_1\n");
	if (gpio_request(113, "ulpi_data_2"))
		pr_err("failed to request gpio ulpi_data_2\n");
	if (gpio_request(114, "ulpi_data_3"))
		pr_err("failed to request gpio ulpi_data_3\n");
	if (gpio_request(115, "ulpi_data_4"))
		pr_err("failed to request gpio ulpi_data_4\n");
	if (gpio_request(116, "ulpi_data_5"))
		pr_err("failed to request gpio ulpi_data_5\n");
	if (gpio_request(117, "ulpi_data_6"))
		pr_err("failed to request gpio ulpi_data_6\n");
	if (gpio_request(118, "ulpi_data_7"))
		pr_err("failed to request gpio ulpi_data_7\n");
	if (gpio_request(119, "ulpi_dir"))
		pr_err("failed to request gpio ulpi_dir\n");
	if (gpio_request(120, "ulpi_next"))
		pr_err("failed to request gpio ulpi_next\n");
	if (gpio_request(121, "ulpi_stop"))
		pr_err("failed to request gpio ulpi_stop\n");
}

static unsigned usb_gpio_lpm_config[] = {
	GPIO_CFG(111, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 0 */
	GPIO_CFG(112, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 1 */
	GPIO_CFG(113, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 2 */
	GPIO_CFG(114, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 3 */
	GPIO_CFG(115, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 4 */
	GPIO_CFG(116, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 5 */
	GPIO_CFG(117, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 6 */
	GPIO_CFG(118, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DATA 7 */
	GPIO_CFG(119, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* DIR */
	GPIO_CFG(120, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),	/* NEXT */
	GPIO_CFG(121, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* STOP */
};

static unsigned usb_gpio_lpm_unconfig[] = {
	GPIO_CFG(111, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 0 */
	GPIO_CFG(112, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 1 */
	GPIO_CFG(113, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 2 */
	GPIO_CFG(114, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 3 */
	GPIO_CFG(115, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 4 */
	GPIO_CFG(116, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 5 */
	GPIO_CFG(117, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 6 */
	GPIO_CFG(118, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DATA 7 */
	GPIO_CFG(119, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DIR */
	GPIO_CFG(120, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* NEXT */
	GPIO_CFG(121, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), /* STOP */
};

static int usb_config_gpio(int config)
{
	int pin, rc;

	if (config) {
		for (pin = 0; pin < ARRAY_SIZE(usb_gpio_lpm_config); pin++) {
			rc = gpio_tlmm_config(usb_gpio_lpm_config[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, usb_gpio_lpm_config[pin], rc);
				return -EIO;
			}
		}
	} else {
		for (pin = 0; pin < ARRAY_SIZE(usb_gpio_lpm_unconfig); pin++) {
			rc = gpio_tlmm_config(usb_gpio_lpm_unconfig[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, usb_gpio_lpm_config[pin], rc);
				return -EIO;
			}
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-08-02, for LGE USB driver */
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-22, new LG USB driver: mass_storage <-> adb */
#ifdef LGE_USB_DRIVER
	{"modem", 0},
	{"diag", 1},	
	{"nmea", 2},
	{"mass_storage", 3},
	{"adb", 4},
	{"ethernet", 5},
#else
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
#endif
/* LGE_CHANGE_E [ljmblueday@lge.com] 2009-08-02 */	
};

/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-07, not used */
#if 0
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9015,
		.functions          = 0x12, /* 10010 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

	{
		.product_id         = 0x901A,
		.functions          = 0x0F, /* 01111 */
	},
#endif
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-08-02, for LGE USB driver */
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-22, new LG USB driver: mass_storage <-> adb */
#ifdef LGE_USB_DRIVER		
	{
		.product_id 		  = 0x6000,
		.functions			  = 0x03 /* 000011 	Diag, Modem*/
	},
	{
		.product_id 		  = 0x6004,
		.functions			  = 0x10, /* 10000 	ADB only*/
	},
	{
		.product_id 		  = 0x6002,
		.functions			  = 0x13, /* 10011 	Diag, Modem, ADB*/
	},
	{
		.product_id 		  = 0x6003,
		.functions			  = 0x02, /* 00010 	Diag only*/
	},
	{
		.product_id 		  = 0x6001,
		.functions			  = 0x08, /* 01000 	mass storage only*/
	},
	{
		.product_id 		  = 0x6005,
		.functions			  = 0x18, /* 11000 	mass storage, ADB*/
	},	
	{
		.product_id 		  = 0x6171,
		.functions			  = 0x12, /* 10010 	Diag, ADB*/
	},	
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-19, new LG Android USB Driver */
	{
		.product_id 		  = 0x618E,
		.functions		  = 0x1F, /* 11111 	Diag, Modem, Nmea, mass storage, ADB*/
	},	
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-07, Dummy */
	{
		.product_id               = 0x6008,
		.functions		  = 0x0F, /* 01111 	Diag, Modem, Nmea, mass storage */
	}
#endif	
/* LGE_CHANGE_E [ljmblueday@lge.com] 2009-08-02 */

};
#endif

#ifdef CONFIG_USB_ANDROID
static void hsusb_phy_reset(void)
{
	msm_hsusb_phy_reset();
}
#endif

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
#ifdef CONFIG_USB_ANDROID
	.phy_reset	= hsusb_phy_reset,
#endif
#ifdef CONFIG_USB_FUNCTION
	.version	= 0x0100,
	.phy_info	= USB_PHY_EXTERNAL,
/* LGE_CHANGE_S [ljmblueday@lge.com] 2009-06-16, for LGE USB driver */	
#ifdef LGE_USB_DRIVER
	.vendor_id			= 0x1004,
	.product_name	  = "LG Mobile USB Modem",
	.serial_number      = NULL, // "1234567890ABCDEF",	
	.manufacturer_name  = "LG Electronics Inc.",	
#else		
	.vendor_id			  = 0x5c6,
	.product_name		  = "Qualcomm HSUSB Device",
	.serial_number      = "1234567890ABCDEF",
	.manufacturer_name  = "Qualcomm Incorporated",
#endif	
/* LGE_CHANGE_E [ljmblueday@lge.com] 2009-06-16 */
	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.ulpi_data_1_pin = 112,
	.ulpi_data_3_pin = 114,
	.config_gpio 	= usb_config_gpio,
#endif
};

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(HEADSET, 2),
	SND(HEADSET_STEREO, 3),
	SND(SPEAKER_MEDIA, 5),
	SND(SPEAKER, 6),
	SND(SPEAKER_RING, 7),
	SND(HEADSET_AND_SPEAKER, 7),
	SND(VOICE_RECORDER, 8),
	SND(FM_RADIO_HEADSET_MEDIA, 9),
	SND(FM_RADIO_SPEAKER_MEDIA, 10),
	SND(BT, 12),

#if defined(CONFIG_MACH_EVE)
	SND(CURRENT, 25),
#else
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(CURRENT, 27),
#endif /* CONFIG_MACH_EVE */
};
#undef SND

static struct msm_snd_endpoints eve_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device eve_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &eve_snd_endpoints
	},
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BUDDYBESTFIT,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_camera_pdata = {
	.name = "pmem_camera",
	.allocator_type = PMEM_ALLOCATORTYPE_BUDDYBESTFIT,
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BUDDYBESTFIT,
	.cached = 0,
};

#ifdef CONFIG_MSM_STACKED_MEMORY
static struct android_pmem_platform_data android_pmem_gpu0_pdata = {
	.name = "pmem_gpu0",
	.start = MSM_PMEM_GPU0_BASE,
	.size = MSM_PMEM_GPU0_SIZE,
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 0,
};
#endif

static struct android_pmem_platform_data android_pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_camera_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_camera_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

#ifdef CONFIG_MSM_STACKED_MEMORY
static struct platform_device android_pmem_gpu0_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_gpu0_pdata },
};
#endif

static struct platform_device android_pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_gpu1_pdata },
};

static struct platform_device eve_battery_device = {
	.name = "lge_battery",
	.id   = -1,
};

/* LGE_CHANGE_S [jh.koo@lge.com] 2009-05-23 */
#if defined(CONFIG_MACH_EVE)
static char *msm_fb_vreg[] = {
        "gp1",           
        "gp2",           
};              
#else
static char *msm_fb_vreg[] = {
        "gp1",           
        "gp2",           
        "gp3",           
        "gp4",           
};              
#endif
/* LGE_CHANGE_E [jh.koo@lge.com] 2009-05-23 */

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.name = "ram_console",
		.start = EVE_RAM_CONSOLE_BASE,
		.end = EVE_RAM_CONSOLE_BASE + EVE_RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resource),
	.resource       = ram_console_resource,
};
#endif

#define MSM_FB_VREG_OP(name, op)                                        \
        do {vreg = vreg_get(0, name);                                   \
        if (vreg_##op(vreg))                                            \
                printk(KERN_ERR "%s: %s vreg operation failed \n",      \
                (vreg_##op == vreg_enable) ? "vreg_enable" : "vreg_disable",\
                name); } while (0)
        
static void msm_fb_mddi_power_save(int on)
{               
        struct vreg *vreg;
        int i;

        for (i = 0; i < ARRAY_SIZE(msm_fb_vreg); i++) {
                if (on) 
                        MSM_FB_VREG_OP(msm_fb_vreg[i], enable);
                else
                        MSM_FB_VREG_OP(msm_fb_vreg[i], disable);
        }

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-27, no delay */
	/*
	if(on){}
	else {
		printk("%s() delay start.\n", __func__);
		mdelay(30);
		printk("%s() delay end.\n", __func__);
	}
	*/
/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-09-19, Set GPIO config */
	if (on)
	{
		gpio_tlmm_config(GPIO_CFG(100, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	}
	else
	{
		//gpio_tlmm_config(GPIO_CFG(100, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE); // LCD_RESET_N
		gpio_tlmm_config(GPIO_CFG(100, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE); // LCD_RESET_N
	}
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009-09-19 */
}               
        
#define PM_VID_EN_CONFIG_PROC          24
#define PM_VID_EN_API_PROG             0x30000061
#define PM_VID_EN_API_VERS             0x00010001 
        
static struct msm_rpc_endpoint *pm_vid_en_ep;
        
static int msm_fb_pm_vid_en(int on)
{
        int rc = 0;
        struct msm_fb_pm_vid_en_req {
                struct rpc_request_hdr hdr;
                uint32_t on;
        } req;
        
        pm_vid_en_ep = msm_rpc_connect(PM_VID_EN_API_PROG,
                                        PM_VID_EN_API_VERS, 0);
        if (IS_ERR(pm_vid_en_ep)) {
                printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",
                        __func__, PTR_ERR(pm_vid_en_ep));
                return -EINVAL;
        }

        req.on = cpu_to_be32(on);
        rc = msm_rpc_call(pm_vid_en_ep,
                        PM_VID_EN_CONFIG_PROC,
                        &req, sizeof(req),
                        5 * HZ);
        if (rc)
                printk(KERN_ERR
                        "%s: msm_rpc_call failed! rc = %d\n", __func__, rc);
                
        msm_rpc_close(pm_vid_en_ep);
        return rc;
} 

static struct tvenc_platform_data tvenc_pdata = {
        .pm_vid_en = msm_fb_pm_vid_en,
};

static struct mddi_platform_data mddi_pdata = {
        .mddi_power_save = msm_fb_mddi_power_save,
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
};

static int mddi_innotek_backlight_level(int level, int max, int min)
{
	/* TODO: return valid backlight level */
	return -1;
}

static struct msm_panel_common_pdata mddi_innotek_pdata = {
	.backlight_level = mddi_innotek_backlight_level,
};

struct platform_device mddi_innotek_device = {
	.name   = "mddi_innotek_hvga",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_innotek_pdata,
	}
};

#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_WAKE,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_PCM_DOUT,
	BT_PCM_DIN,
	BT_PCM_SYNC,
	BT_PCM_CLK,
	BT_HOST_WAKE,
};
// shyoo modify
static unsigned bt_config_power_on[] = {
	GPIO_CFG(92, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* WAKE */
	GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* RFR */
	GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* CTS */
	GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* Rx */
	GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* Tx */
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* HOST_WAKE */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(92, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* WAKE */
	GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* RFR */
	GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* CTS */
	GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* Rx */
	GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* Tx */
	GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* HOST_WAKE */
};
// original code
#if 0
static unsigned bt_config_power_on[] = {
	GPIO_CFG(42, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* HOST_WAKE */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(42, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* HOST_WAKE */
};
#endif

#define	GPIO_BT_RESET_N		93
#define	GPIO_BT_WAKEUP		92
#define	GPIO_BT_HOST_WAKEUP	83
#define	GPIO_BT_REG_ON		21 //19
#define	GPIO_WL_RESET_N		35

static int bluetooth_power(int on)
{
	int pin, rc;

	printk(KERN_DEBUG "%s(%d)\n", __func__, on);
    printk(KERN_DEBUG "on_off: %d\n", on);  //added by shyoo

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
						"%s: gpio_tlmm_config(%#x)=%d\n",
						__func__, bt_config_power_on[pin], rc);
				return -EIO; 
			}
		}

		printk(KERN_DEBUG	"*****wait for regulator turned-on, 150ms*****\n");
		
#if 1  //fm White
/*turn-on regulator*/
if(!gpio_get_value(GPIO_BT_REG_ON))
	gpio_set_value(GPIO_BT_REG_ON, 1);

/*wait for regulator turned-on, 200ms*/
msleep (200);

/*drive /RESET pin to LOW*/
gpio_set_value(GPIO_BT_RESET_N, 0);
/*reset assert for 120ms*/
msleep (120);

/*Power On Reset BCM4325D0*/
gpio_set_value(GPIO_BT_RESET_N, 1);
/*wait for 50ms for POR BCM2045*/
msleep (50);
#else
		/*turn-on regulator*/
		if(!gpio_get_value(GPIO_BT_REG_ON))
			gpio_set_value(GPIO_BT_REG_ON, 1);

		/*drive /RESET pin to LOW*/
		gpio_set_value(GPIO_BT_RESET_N, 0);
		/*wait for regulator turned-on, 150ms*/
		msleep (150);
		/*Power On Reset BCM4325D0*/
		gpio_set_value(GPIO_BT_RESET_N, 1);
		/*wait for 20ms for POR BCM2045*/
		msleep (20);
#endif		
	} else {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
						"%s: gpio_tlmm_config(%#x)=%d\n",
						__func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
		gpio_set_value(GPIO_BT_RESET_N, 0);
		if(!gpio_get_value(GPIO_WL_RESET_N))
			gpio_set_value(GPIO_BT_REG_ON, 0);
	}
	return 0;
}

static void __init bt_power_init(void)
{
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 92,
		.end	= 92,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(83),
		.end	= MSM_GPIO_TO_INT(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static struct gpio_switch_platform_data eve_hs_pdata = {
#define GPIO_EAR_SENSE	29
	.gpio = GPIO_EAR_SENSE,
	.name = "h2w",
};

static struct platform_device eve_hs_device = {
	.name = "switch-gpio",
	.id = -1,
	.dev    = {
		.platform_data = &eve_hs_pdata
	},
}; 

static struct platform_device android_keyled_device = {
	.name 	= "android-keyled",
	.id	= -1,
	.dev = {
		.platform_data = 0,
	},
};

static struct platform_device android_hallic_device = {
	.name 	= "hall-ic",
	.id	= -1,
	.dev = {
		.platform_data = 0,
	},
};

static struct platform_device android_vibrator_device = {
	.name 	= "android-vibrator",
	.id	= -1,
	.dev = {
		.platform_data = 0,
	},
};

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-26, not used */
#if 0
static struct platform_device eve_keypad_device = {
	.name	= "eve_keypad",
	.id	= -1,
	.dev	= {
		.platform_data	= 0,
	},
};
#endif

static struct platform_device eve_qwerty_device = {
	.name	= "eve_qwerty",
	.id	= -1,
	.dev	= {
		.platform_data	= 0,
	},
};

static struct platform_device eve_camera_flashlight_device = {
        .name   = "camera_flash_led",
        .id     = -1,
        .dev = {
                .platform_data = 0,
        },
};

#if 1 //vlc
static struct atcmd_platform_data eve_atcmd_pdata = {
	.name = "eve_atcmd",
};

static struct platform_device eve_atcmd_device = {
	.name = "eve_atcmd",
	.id = -1,
	.dev    = {
		.platform_data = &eve_atcmd_pdata
	},
}; 
#endif
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design   = 3200,
	.voltage_max_design  = 4200,
	.avail_chg_sources   = AC_CHG | USB_CHG ,
	.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity  = &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
	/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name     = "msm-battery",
	.id    = -1,
	.dev.platform_data = &msm_psy_batt_data,
};

static struct platform_device *devices[] __initdata = {
#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
	&msm_device_uart_dm1,
	&msm_device_smd,
	&msm_device_nand,
	&msm_device_i2c,
	&msm_device_tssc,
	&android_pmem_camera_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&eve_battery_device,
#ifdef CONFIG_MSM_STACKED_MEMORY
	&android_pmem_gpu0_device,
#endif
	&android_pmem_gpu1_device,
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
	&msm_device_hsusb_otg,
	&msm_device_hsusb_host,
#if defined(CONFIG_USB_FUNCTION) || defined(CONFIG_USB_ANDROID)
	&msm_device_hsusb_peripheral,
#endif
#ifdef CONFIG_USB_FUNCTION
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID
	&android_usb_device,
#endif

#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
	&msm_bluesleep_device,
	&msm_fb_device,
	&mddi_innotek_device,
	&msm_batt_device,
	
	/* LGE_CHANGE [cleaneye@lge.com] 2009-02-23, eve specific devices */
	&eve_snd,
	&android_hallic_device, 	//hall ic
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-26, not used */
	//&eve_keypad_device, //gpio_key_in side
	&eve_hs_device,
	&android_vibrator_device, //vibrator
	&android_keyled_device, //keyled
	&eve_qwerty_device,
	&eve_camera_flashlight_device,
	&eve_atcmd_device, //vlc
	&pwrkey_device,
};

#define EVE_GPIO_PS_HOLD	(25)
void (*msm_hw_reset_hook)(void);

static void eve_reset(void)
{

	printk(KERN_INFO"%s: %d\n",__func__, gpio_get_value(EVE_GPIO_PS_HOLD));
	gpio_set_value(EVE_GPIO_PS_HOLD, 0);
	printk(KERN_INFO"%s: %d\n",__func__, gpio_get_value(EVE_GPIO_PS_HOLD));
}


extern struct sys_timer msm_timer;

/* LGE_CHANGE [cleaneye@lge.com] 2009-02-24, temporary blocking */
/* the following are codes about qualcomm surf specific camera device driver
 * so these codes may not work well.
 * moreover, there is possibility of side-effect related to GPIO setting.
 */
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	//GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
   /* parallel CAMERA interfaces */
       GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
       GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
       GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
       GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
       GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
       GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
       GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
       GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
       GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
       GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
       GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
       GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void config_camera_on_gpios(void)
{
	printk("msm_camera: config camera gpio ON\n");
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	printk("msm_camera: config camera gpio OFF\n");
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

#define MSM_PROBE_INIT(name) name##_probe_init
static struct msm_camera_sensor_info msm_camera_sensor[] = {
	{
		.sensor_reset = 0,
		.sensor_pwd   = 0,
		.vcm_pwd      = 0,
		.sensor_name  = "mv9319_sony",
		.flash_type		= MSM_CAMERA_FLASH_NONE,
#ifdef CONFIG_MSM_CAMERA
		.sensor_probe = MSM_PROBE_INIT(mv9319),
#endif
	},
};
#undef MSM_PROBE_INIT

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.snum = ARRAY_SIZE(msm_camera_sensor),
	.sinfo = &msm_camera_sensor[0],
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

static void __init msm_camera_add_device(void)
{
	msm_camera_register_device(NULL, 0, &msm_camera_device_data);
	config_camera_off_gpios();
}

static void __init eve_init_irq(void)
{
	msm_init_irq();
}

static struct msm_acpu_clock_platform_data eve_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.power_collapse_khz = 19200000,
	.wait_for_irq_khz = 128000000,
	.max_axi_khz = 128000,
};

void msm_serial_debug_init(unsigned int base, int irq,
				struct device *clk_device, int signal_irq);
static void sdcc_gpio_init(void)
{
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	int rc = 0;
	if (gpio_request(GPIO_MMC_CD_N, "sdc2_status_irq"))
		pr_err("failed to request gpio sdc2_status_irq\n");
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_MMC_CD_N, 0, GPIO_INPUT, GPIO_PULL_UP,
				GPIO_2MA), GPIO_ENABLE);
	if (rc)
		printk(KERN_ERR "%s: Failed to configure GPIO %d\n",
				__func__, rc);
#endif
	/* SDC1 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (gpio_request(51, "sdc1_data_3"))
		pr_err("failed to request gpio sdc1_data_3\n");
	if (gpio_request(52, "sdc1_data_2"))
		pr_err("failed to request gpio sdc1_data_2\n");
	if (gpio_request(53, "sdc1_data_1"))
		pr_err("failed to request gpio sdc1_data_1\n");
	if (gpio_request(54, "sdc1_data_0"))
		pr_err("failed to request gpio sdc1_data_0\n");
	if (gpio_request(55, "sdc1_cmd"))
		pr_err("failed to request gpio sdc1_cmd\n");
	if (gpio_request(56, "sdc1_clk"))
		pr_err("failed to request gpio sdc1_clk\n");
#endif

	if (machine_is_msm7201a_ffa())
		return;

	/* SDC2 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (gpio_request(62, "sdc2_clk"))
		pr_err("failed to request gpio sdc2_clk\n");
	if (gpio_request(63, "sdc2_cmd"))
		pr_err("failed to request gpio sdc2_cmd\n");
	if (gpio_request(64, "sdc2_data_3"))
		pr_err("failed to request gpio sdc2_data_3\n");
	if (gpio_request(65, "sdc2_data_2"))
		pr_err("failed to request gpio sdc2_data_2\n");
	if (gpio_request(66, "sdc2_data_1"))
		pr_err("failed to request gpio sdc2_data_1\n");
	if (gpio_request(67, "sdc2_data_0"))
		pr_err("failed to request gpio sdc2_data_0\n");
#endif

	/* SDC4 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	if (gpio_request(19, "sdc4_data_3"))
		pr_err("failed to request gpio sdc4_data_3\n");
	if (gpio_request(20, "sdc4_data_2"))
		pr_err("failed to request gpio sdc4_data_2\n");
	if (gpio_request(21, "sdc4_data_1"))
		pr_err("failed to request gpio sdc4_data_1\n");
	if (gpio_request(107, "sdc4_cmd"))
		pr_err("failed to request gpio sdc4_cmd\n");
	if (gpio_request(108, "sdc4_data_0"))
		pr_err("failed to request gpio sdc4_data_0\n");
	if (gpio_request(109, "sdc4_clk"))
		pr_err("failed to request gpio sdc4_clk\n");
#endif
}

static unsigned sdcc_cfg_data[][6] = {
	/* SDC1 configs */
	{
	GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	},
	/* SDC2 configs */
	{
	GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	{
	/* SDC3 configs */
	},
	/* SDC4 configs */
	{
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-05, not used */
#if 0
	GPIO_CFG(19, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(20, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(21, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(107, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(108, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(109, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
#endif
	}
};

static unsigned long vreg_sts, gpio_sts;
static struct mpp *mpp_mmc;
static struct vreg *vreg_mmc;

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int i, rc;

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable)
		set_bit(dev_id, &gpio_sts);
	else
		clear_bit(dev_id, &gpio_sts);

	for (i = 0; i < ARRAY_SIZE(sdcc_cfg_data[dev_id - 1]); i++) {
		rc = gpio_tlmm_config(sdcc_cfg_data[dev_id - 1][i],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, sdcc_cfg_data[dev_id - 1][i], rc);
		}
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			if (machine_is_msm7201a_ffa())
				rc = mpp_config_digital_out(mpp_mmc,
				     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     MPP_DLOGIC_OUT_CTRL_LOW));
			else
				rc = vreg_disable(vreg_mmc);
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		if (machine_is_msm7201a_ffa())
			rc = mpp_config_digital_out(mpp_mmc,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));
		else {
			rc = vreg_set_level(vreg_mmc, 2850);
			if (!rc)
				rc = vreg_enable(vreg_mmc);
		}
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int eve_sdcc_slot_status(struct device *dev)
{
	return !gpio_get_value(GPIO_MMC_CD_N);
}
#endif
/* LGE_CHANGE_S [yoohoo@lge.com] 2009-03-05, BCM4325 control gpio */
#ifdef CONFIG_LGE_BCM432X_PATCH
static unsigned int eve_sdcc_wlan_slot_status(struct device *dev)
{
	return gpio_get_value(CONFIG_BCM4325_GPIO_WL_RESET);
}

static struct mmc_platform_data eve_sdcc_wlan_data = {
	.ocr_mask	= MMC_VDD_30_31,
	.translate_vdd	= msm_sdcc_setup_power,
	.status		= eve_sdcc_wlan_slot_status,
	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
};
#endif /* CONFIG_LGE_BCM432X_PATCH*/
/* LGE_CHANGE_E [yoohoo@lge.com] 2009-03-05, BCM4325 control gpio */
static struct mmc_platform_data eve_sdcc_data = {
	.ocr_mask	= MMC_VDD_30_31,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status         = eve_sdcc_slot_status,
	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
};

static void __init eve_init_mmc(void)
{
	if (machine_is_msm7201a_ffa()) {
		mpp_mmc = mpp_get(NULL, "mpp3");
		if (!mpp_mmc) {
			printk(KERN_ERR "%s: mpp get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	} else {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}

	sdcc_gpio_init();
/* LGE_CHANGE_S [yoohoo@lge.com] 2009-03-05, BCM4325 control gpio */
#ifdef CONFIG_LGE_BCM432X_PATCH
	/* countrol output */
#ifdef CONFIG_BCM4325_GPIO_WL_REGON
	gpio_direction_output(CONFIG_BCM4325_GPIO_WL_REGON, 0);
#endif /* CONFIG_BCM4325_GPIO_WL_REGON */
	gpio_direction_output(CONFIG_BCM4325_GPIO_WL_RESET, 0);
	mdelay(150);

	gpio_request(CONFIG_BCM4325_GPIO_WL_RESET, "wlan_cd");

	msm_add_sdcc(1, &eve_sdcc_wlan_data, gpio_to_irq(CONFIG_BCM4325_GPIO_WL_RESET), IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING);
#else /* CONFIG_LGE_BCM432X_PATCH */
	msm_add_sdcc(1, &eve_sdcc_data, 0, 0);
#endif /* CONFIG_LGE_BCM432X_PATCH */
/* LGE_CHANGE_E [yoohoo@lge.com] 2009-03-05, BCM4325 control gpio */

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-05, for new kernel */
	msm_add_sdcc(2, &eve_sdcc_data, MSM_GPIO_TO_INT(GPIO_MMC_CD_N),
			(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING));
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-05, not used */
	//msm_add_sdcc(4, &eve_sdcc_data, 0, 0);
#endif
}

static void __init msm_fb_add_devices(void)
{
        msm_fb_register_device("mdp", 0);
        msm_fb_register_device("ebi2", 0);
        msm_fb_register_device("pmdh", &mddi_pdata);
        msm_fb_register_device("emdh", 0);
        msm_fb_register_device("tvenc", &tvenc_pdata);
}

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
};

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-08-29 */
#if defined(CONFIG_MACH_EVE)
// Please call this function only once.
void eve_get_hw_rev(void)
{
	int lge_hw_rev = LGE_PCB_VER_UNKNOWN;

	if (0 != msm_proc_comm_get_hw_rev(&lge_hw_rev))
	{
		// if msm_proc_comm err
		lge_hw_rev = LGE_PCB_VER_UNKNOWN;
		printk("[HW] %s() msm_proc_comm error\n", __func__);
	}
	if (-1 == lge_hw_rev) // Fail
	{
		lge_hw_rev = LGE_PCB_VER_UNKNOWN;
		printk("[HW] %s() Fail to get data at modem side\n", __func__);
	}
	
	printk("[HW] %s() HW version is %d\n", __func__, lge_hw_rev);

	system_rev = (unsigned int)lge_hw_rev;
}
#endif
/* LGE_CHANGE_S [jinwoonam@lge.com] 2009-08-29 */

	/* LGE_CHANGE_S [diyu@lge.com] 2009-05-11,  */
extern void eve_init_accel(void);
extern void eve_init_touch(void);
extern void  eve_init_prox(void);
	/* LGE_CHANGE_E [diyu@lge.com] 2009-05-11,  */

int eve_init_i2c_backlight(void);
void eve_init_i2c_amp(void);
void eve_init_i2c_camera(void);
void eve_init_i2c_compass(void);

static void __init eve_init(void)
{

	if (socinfo_init() < 0)
		BUG();

	eve_get_hw_rev();

	/* All 7x01 2.0 based boards are expected to have RAM chips capable
	 * of 160 MHz. */
	if (cpu_is_msm7x01()
	    && SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2)
		eve_clock_data.max_axi_khz = 160000;

#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
			      &msm_device_uart3.dev, 1);
#endif
	msm_hsusb_pdata.soc_version = socinfo_get_version();
	msm_acpu_clock_init(&eve_clock_data);
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata,
	msm_device_hsusb_host.dev.platform_data = &msm_hsusb_pdata,
	platform_add_devices(devices, ARRAY_SIZE(devices));

#ifdef CONFIG_EVE_GPIO_KEYPAD
	platform_device_register(&keypad_device_eve);
#endif

	eve_init_mmc();
	eve_init_i2c_backlight();
 	eve_init_i2c_amp();
	eve_init_i2c_camera();
	msm_camera_add_device();
#ifdef CONFIG_USB_FUNCTION
	hsusb_gpio_init();
#endif
	msm_fb_add_devices();

	bt_power_init();

#ifdef CONFIG_USB_ANDROID
	msm_hsusb_rpc_connect();
	msm_hsusb_set_vbus_state(1) ;
#endif
	/* LGE_CHANGE [cleaneye@lge.com] 2009-02-23, eve specific devices */
	eve_init_accel();  // G-Sensor BMA150
	eve_init_i2c_compass();	// Compass
	eve_init_touch(); // for capacitive touch
	eve_init_prox();  // Proximity Sensor
	msm_pm_set_platform_data(msm_pm_data);
	msm_hw_reset_hook = eve_reset;
}

static void __init eve_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].node = PHYS_TO_NID(PHYS_OFFSET);
#if CONFIG_ANDROID_RAM_CONSOLE
/* decrease system memory size for allocating ram_console buffer region
 * 224Mbyte -> 223Mbyte
 * 2009/05/26 cleaneye@lge.com */
	mi->bank[0].size = (223*1024*1024);
#else
/* LGE_CHANGE [parksh03@lge.com] 2009-03-09, change the size of the system memory */
	mi->bank[0].size = (224*1024*1024);
#endif
}

static void __init msm_eve_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = MSM_PMEM_MDP_SIZE;
	addr = alloc_bootmem(size);
	android_pmem_pdata.start = __pa(addr);
	android_pmem_pdata.size = size;
	printk(KERN_INFO "allocating %lu bytes at %p (%lx physical)"
	       "for pmem\n", size, addr, __pa(addr));

	size = MSM_PMEM_CAMERA_SIZE;
	addr = alloc_bootmem(size);
	android_pmem_camera_pdata.start = __pa(addr);
	android_pmem_camera_pdata.size = size;
	printk(KERN_INFO "allocating %lu bytes at %p (%lx physical)"
	       "for camera pmem\n", size, addr, __pa(addr));

	size = MSM_PMEM_ADSP_SIZE;
	addr = alloc_bootmem(size);
	android_pmem_adsp_pdata.start = __pa(addr);
	android_pmem_adsp_pdata.size = size;
	printk(KERN_INFO "allocating %lu bytes at %p (%lx physical)"
	       "for adsp pmem\n", size, addr, __pa(addr));

	size = MSM_PMEM_GPU1_SIZE;
	addr = alloc_bootmem_aligned(size, 0x100000);
	android_pmem_gpu1_pdata.start = __pa(addr);
	android_pmem_gpu1_pdata.size = size;
	printk(KERN_INFO "allocating %lu bytes at %p (%lx physical)"
	       "for gpu1 pmem\n", size, addr, __pa(addr));

	size = MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	printk(KERN_INFO "allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));

}

static void __init eve_map_io(void)
{
	msm_shared_ram_phys = 0x01F00000;

	msm_map_common_io();
        /* LGE_CHANGE_S [zugwan@lge.com] 2009-05-22, add mapping for the vibrator */
	msm_map_eve_io();
        /* LGE_CHANGE_S [zugwan@lge.com] 2009-05-22, add mapping for the vibrator */
	msm_clock_init(msm_clocks_7x01a, msm_num_clocks_7x01a);

	msm_eve_allocate_memory_regions();
}

MACHINE_START(EVE, "Eve Board (LGE GW650)")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= 0x10000100,
	.fixup		= eve_fixup,
	.map_io		= eve_map_io,
	.init_irq	= eve_init_irq,
	.init_machine	= eve_init,
	.timer		= &msm_timer,
MACHINE_END
