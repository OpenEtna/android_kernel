/* linux/arch/arm/mach-msm/board-eve.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
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

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>

#include <mach/irqs.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/msm_fb.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include <mach/msm_rpcrouter.h>
#include <mach/bcm_bt_lpm.h>
#include <mach/msm_serial_hs.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/android_pmem.h>
#include <linux/usb/android_composite.h>

#include "devices.h"
#include "board-eve.h"
#include "proc_comm.h"

/* hw3d */
static struct resource resources_hw3d[] = {
	{
		.start	= 0xA0000000,
		.end	= 0xA00fffff,
		.flags	= IORESOURCE_MEM,
		.name	= "regs",
	},
	{
		.flags	= IORESOURCE_MEM,
                .start  = MSM_PMEM_GPU0_BASE,
                .end    = MSM_PMEM_GPU0_BASE+MSM_PMEM_GPU0_SIZE-1,
		.name	= "smi",
	},
	{
		.flags	= IORESOURCE_MEM,
                .start  = MSM_PMEM_GPU1_BASE,
                .end    = MSM_PMEM_GPU1_BASE+MSM_PMEM_GPU1_SIZE-1,
		.name	= "ebi",
	},
	{
		.start	= INT_GRAPHICS,
		.end	= INT_GRAPHICS,
		.flags	= IORESOURCE_IRQ,
		.name	= "gfx",
	},
};

static struct platform_device hw3d_device = {
	.name		= "msm_hw3d",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_hw3d),
	.resource	= resources_hw3d,
};

/* ear jack sense */
static struct gpio_switch_platform_data eve_hs_pdata = {
#define GPIO_EAR_SENSE  29
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

/* sound */
#define SND(desc, num) { .name = #desc, .id = num }
/* original mapping from lg
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
    SND(CURRENT, 25),
*/
static struct snd_endpoint snd_endpoints_list[] = {
    SND(HANDSET, 0),
    SND(HEADSET, 3), //use HEADSET_STEREO's id
    SND(SPEAKER, 6), //using SPEAKER_MEDIA's id would produce bug issue 262
    SND(HEADSET_AND_SPEAKER, 7),
    SND(FM_HEADSET, 9),
    SND(FM_SPEAKER, 10),
    SND(BT, 12),
    SND(CURRENT, 25),
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

/* pmem */
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.start = MSM_PMEM_MDP_BASE,
	.size = MSM_PMEM_MDP_SIZE,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
    .name = "pmem_adsp",
    .cached = 0,
    .start = MSM_PMEM_ADSP_BASE,
    .size = MSM_PMEM_ADSP_SIZE,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
    .name = "android_pmem",
    .id = 1,
    .dev = { .platform_data = &android_pmem_adsp_pdata },
};

/* last_kmesg */
static struct resource ram_console_resources[] = {
    {
        .start  = MSM_RAM_CONSOLE_BASE,
        .end    = MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
        .flags  = IORESOURCE_MEM,
    },
};

static struct platform_device ram_console_device = {
    .name       = "ram_console",
    .id     = -1,
    .num_resources  = ARRAY_SIZE(ram_console_resources),
    .resource   = ram_console_resources,
};

/* usb */
#define HSUSB_API_INIT_PHY_PROC 2
#define HSUSB_API_PROG      0x30000064
#define HSUSB_API_VERS MSM_RPC_VERS(1,1)

static void internal_phy_reset(void)
{
    struct msm_rpc_endpoint *usb_ep;
    int rc;
    struct hsusb_phy_start_req {
        struct rpc_request_hdr hdr;
    } req;

    printk(KERN_INFO "msm_hsusb_phy_reset\n");

    usb_ep = msm_rpc_connect(HSUSB_API_PROG, HSUSB_API_VERS, 0);
    if (IS_ERR(usb_ep)) {
        printk(KERN_ERR "%s: init rpc failed! error: %ld\n",
                __func__, PTR_ERR(usb_ep));
        goto close;
    }
    rc = msm_rpc_call(usb_ep, HSUSB_API_INIT_PHY_PROC,
            &req, sizeof(req), 5 * HZ);
    if (rc < 0)
        printk(KERN_ERR "%s: rpc call failed! (%d)\n", __func__, rc);

close:
    msm_rpc_close(usb_ep);
}

/* adjust eye diagram, disable vbusvalid interrupts */
static int hsusb_phy_init_seq[] = { 0x40, 0x31, 0x1D, 0x0D, 0x1D, 0x10, -1 };

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
    .phy_reset = internal_phy_reset,
    .phy_init_seq = hsusb_phy_init_seq,
    .usb_connected = notify_usb_connected,
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
    .nluns = 1,
    .vendor = "HTC     ",
    .product = "Android Phone   ",
    .release = 0x0100,
};

static struct platform_device usb_mass_storage_device = {
    .name = "usb_mass_storage",
    .id = -1,
    .dev = {
        .platform_data = &mass_storage_pdata,
        },
};

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
    /* ethaddr is filled by board_serialno_setup */
    .vendorID   = 0x0bb4,
    .vendorDescr    = "HTC",
};

static struct platform_device rndis_device = {
    .name   = "rndis",
    .id = -1,
    .dev    = {
        .platform_data = &rndis_pdata,
    },
};
#endif

static char *usb_functions_ums[] = {
    "usb_mass_storage",
};

static char *usb_functions_ums_adb[] = {
    "usb_mass_storage",
    "adb",
};

static char *usb_functions_rndis[] = {
    "rndis",
};

static char *usb_functions_rndis_adb[] = {
    "rndis",
    "adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
    "rndis",
#endif
    "usb_mass_storage",
    "adb",
#ifdef CONFIG_USB_ANDROID_ACM
    "acm",
#endif
};

static struct android_usb_product usb_products[] = {
    {
        .product_id = 0x0c01,
        .num_functions  = ARRAY_SIZE(usb_functions_ums),
        .functions  = usb_functions_ums,
    },
    {
        .product_id = 0x0c02,
        .num_functions  = ARRAY_SIZE(usb_functions_ums_adb),
        .functions  = usb_functions_ums_adb,
    },
    {
        .product_id = 0x0ffe,
        .num_functions  = ARRAY_SIZE(usb_functions_rndis),
        .functions  = usb_functions_rndis,
    },
    {
        .product_id = 0x0ffc,
        .num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
        .functions  = usb_functions_rndis_adb,
    },
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id = 0x1004,
	.product_id = 0x0c01,
	.version = 0x0100,
	.serial_number = "42",
	.product_name = "GW620",
	.manufacturer_name = "LG",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct platform_device android_usb_device = {
	.name = "android_usb",
	.id = -1,
	.dev = {
		.platform_data = &android_usb_pdata,
	},
};

/* bluetooth */
static struct platform_device eve_rfkill = {
	.name = "eve_rfkill",
	.id = -1,
};

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
    .rx_wakeup_irq = -1,
    .inject_rx_on_wakeup = 0,
    .exit_lpm_cb = bcm_bt_lpm_exit_lpm_locked,
};

static struct bcm_bt_lpm_platform_data bcm_bt_lpm_pdata = {
    .gpio_wake = GPIO_BT_WAKE,
    .gpio_host_wake = GPIO_BT_HOST_WAKE,
    .request_clock_off_locked = msm_hs_request_clock_off_locked,
    .request_clock_on_locked = msm_hs_request_clock_on_locked,
};

struct platform_device bcm_bt_lpm_device = {
    .name = "bcm_bt_lpm",
    .id = 0,
    .dev = {
        .platform_data = &bcm_bt_lpm_pdata,
    },
};

/* mmc */
void eve_init_mmc(void);

/* for Handset */
static struct platform_device pwrkey_device = {
	.name   = "msm-handset",
	.id     = -1,
};

/* qwerty keyboard */
static struct platform_device eve_qwerty_device = {
    .name   = "eve_qwerty",
    .id = -1,
};

/* battery and charger info */
static struct platform_device eve_battery_device = {
    .name = "lge_battery",
    .id   = -1,
};

/* vibrator */
static struct platform_device android_vibrator_device = {
    .name   = "android-vibrator",
    .id = -1,
};

/* i2c devices */
/* motion sensor */
static struct i2c_gpio_platform_data accel_i2c_pdata = {
        .sda_pin = GPIO_MOTION_I2C_SDA,
        .sda_is_open_drain = 0,
        .scl_pin = GPIO_MOTION_I2C_SCL,
        .scl_is_open_drain = 0,
        .udelay = 2,
};

static struct platform_device eve_accel_i2c_bus = {
        .name = "i2c-gpio",
        .id = I2C_BUS_NUM_MOTION,
        .dev.platform_data = &accel_i2c_pdata,
};

static struct i2c_board_info accel_i2c_bdinfo = {
        I2C_BOARD_INFO("bma150", 0x38),
        .type = "bma150",
        .irq = MSM_GPIO_TO_INT(GPIO_MOTION_IRQ),
};
/* backlight */
static struct i2c_board_info bl_i2c_device = {
        I2C_BOARD_INFO("backlight", 0xEC >> 1),
};

static struct i2c_gpio_platform_data bl_i2c_adap_pdata = {
        .sda_pin = GPIO_BL_I2C_SDA,
        .sda_is_open_drain = 0,
        .scl_pin = GPIO_BL_I2C_SCL,
        .scl_is_open_drain = 0,
        .udelay = 2,
};

static struct platform_device eve_bl_i2c_adap_bus = {
        .name = "i2c-gpio",
        .id = I2C_BUS_NUM_BACKLIGHT,
        .dev.platform_data = &bl_i2c_adap_pdata,
};
/* camera */
static struct i2c_gpio_platform_data eve_camera_i2c_adap_pdata =
{
    .sda_pin= GPIO_CAMERA_I2C_SDA,
    .sda_is_open_drain = 0,
    .scl_pin = GPIO_CAMERA_I2C_SCL,
    .scl_is_open_drain = 0,
    .udelay = 2,
};

static struct platform_device eve_camera_i2c_adap_bus =
{
    .name= "i2c-gpio",
    .id = I2C_BUS_NUM_CAMERA,
    .dev.platform_data = &eve_camera_i2c_adap_pdata,
};

static struct i2c_board_info eve_camera_i2c_device[] =
{
    { I2C_BOARD_INFO("mv9319", 0x50 >> 1) },
    { I2C_BOARD_INFO("mv9319_firmware", 0x40 >> 1) },
};

/* proximity */
static struct i2c_gpio_platform_data prox_i2c_pdata = {
        .sda_pin = GPIO_PROX_I2C_SDA,
        .sda_is_open_drain = 0,
        .scl_pin = GPIO_PROX_I2C_SCL,
        .scl_is_open_drain = 0,
        .udelay = 2,
};

static struct platform_device eve_prox_i2c_bus = {
        .name = "i2c-gpio",
        .id = I2C_BUS_NUM_PROX,
        .dev.platform_data = &prox_i2c_pdata,
};

static struct i2c_board_info prox_i2c_bdinfo = {
        I2C_BOARD_INFO("gp2ap002", 0x44),
        .type = "gp2ap002",
        .irq = GPIO_PROX_IRQ, /* pass the gpio, not the irq */
};

/* compass */
static struct i2c_gpio_platform_data compass_i2c_pdata = {
        .sda_pin = GPIO_COMPASS_I2C_SDA,
        .sda_is_open_drain = 0,
        .scl_pin = GPIO_COMPASS_I2C_SCL,
        .scl_is_open_drain = 0,
        .udelay = 2,
};

static struct platform_device eve_compass_i2c_bus = {
        .name = "i2c-gpio",
        .id = I2C_BUS_NUM_COMPASS,
        .dev.platform_data = &compass_i2c_pdata,
};

static struct i2c_board_info compass_i2c_bdinfo = {
        I2C_BOARD_INFO("akm8973", 0x38 >> 1),
        .irq = MSM_GPIO_TO_INT(GPIO_COMPASS_IRQ),
};

/* Home & Back button */
static struct i2c_gpio_platform_data touch_i2c_pdata = {
        .sda_pin = GPIO_TOUCH_I2C_SDA,
        .sda_is_open_drain = 0,
        .scl_pin = GPIO_TOUCH_I2C_SCL,
        .scl_is_open_drain = 0,
        .udelay = 2,
};

static struct platform_device eve_touch_i2c_bus = {
        .name = "i2c-gpio",
        .id = I2C_BUS_NUM_TOUCH,
        .dev.platform_data = &touch_i2c_pdata,
};

static struct i2c_board_info i2c_board_touch = {
	I2C_BOARD_INFO("touch_so240001", 0x2C),
	.irq = MSM_GPIO_TO_INT(GPIO_TOUCH_IRQ),
};

/* list of all devices from above or devices-msm7x00.c */
static struct platform_device *devices[] __initdata = {
	&ram_console_device,
	&msm_device_uart3,
	&bcm_bt_lpm_device,
	&msm_device_uart_dm1,
	&eve_rfkill,
	&msm_device_smd,
	&msm_device_nand,

	&msm_device_hsusb,
	&usb_mass_storage_device,
	&android_usb_device,
	&rndis_device,
	&eve_battery_device,

	//&msm_device_i2c,
	&msm_device_touchscreen,
	&eve_bl_i2c_adap_bus,

	&eve_camera_i2c_adap_bus,

	&eve_compass_i2c_bus,
	&eve_prox_i2c_bus,
	&eve_accel_i2c_bus,

	&eve_touch_i2c_bus,
	&eve_qwerty_device,
	&pwrkey_device,

	&android_vibrator_device,

	&android_pmem_device,
	&android_pmem_adsp_device,
	&hw3d_device,
	&eve_snd,
	&eve_hs_device,
};

extern struct sys_timer msm_timer;

static struct msm_acpu_clock_platform_data eve_clock_data = {
	.acpu_switch_time_us	= 50,
	.max_speed_delta_khz	=    256000,
	.vdd_switch_time_us		= 62,
	.power_collapse_khz		=  19200000,
	.wait_for_irq_khz		= 128000000,
};

// Please call this function only once.
static void eve_get_hw_rev(void) {
	int lge_hw_rev = LGE_PCB_VER_UNKNOWN;

	int fntype = CUSTOMER_CMD2_BATT_GET_HW_REV;

    if (msm_proc_comm(PCOM_CUSTOMER_CMD2, &lge_hw_rev, &fntype)) {
		lge_hw_rev = LGE_PCB_VER_UNKNOWN;
		printk("[HW] %s() msm_proc_comm error\n", __func__);
	}
	if (-1 == lge_hw_rev) {
		lge_hw_rev = LGE_PCB_VER_UNKNOWN;
		printk("[HW] %s() Fail to get data at modem side\n", __func__);
	}
	printk("[HW] %s() HW version is %d\n", __func__, lge_hw_rev);
	system_rev = (unsigned int)lge_hw_rev;
}

static void __init eve_init(void)
{
	eve_get_hw_rev();

	msm_device_hsusb.dev.platform_data = &msm_hsusb_pdata;
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
	msm_acpu_clock_init(&eve_clock_data);

	/* the chip does not onky handle the backlight, but also
	 * the power supply of the camera and vibrator */
	eve_backlight_init();

	/* register i2c devices */
	/* each pair of SCL and SDA lines is one bus */
	i2c_register_board_info(I2C_BUS_NUM_MOTION, &accel_i2c_bdinfo, 1);
	i2c_register_board_info(I2C_BUS_NUM_BACKLIGHT, &bl_i2c_device, 1);
	i2c_register_board_info(I2C_BUS_NUM_CAMERA, eve_camera_i2c_device, 2);
	i2c_register_board_info(I2C_BUS_NUM_TOUCH, &i2c_board_touch, 1);
	i2c_register_board_info(I2C_BUS_NUM_PROX, &prox_i2c_bdinfo, 1);
	i2c_register_board_info(I2C_BUS_NUM_COMPASS, &compass_i2c_bdinfo, 1);

	gpio_request(GPIO_MOTION_IRQ, "bma150_int");
	gpio_direction_input(GPIO_MOTION_IRQ);

	gpio_request(GPIO_COMPASS_RESET, "compass_rst");
	gpio_direction_output(GPIO_COMPASS_RESET, 1);
	gpio_request(GPIO_COMPASS_IRQ, "compass_int");
	gpio_direction_input(GPIO_COMPASS_IRQ);

	eve_init_mmc();
	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static void __init eve_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].node = PHYS_TO_NID(PHYS_OFFSET);
	mi->bank[0].size = MSM_LINUX_SIZE;
}

static struct map_desc eve_io_desc[] __initdata = {
	{
		.virtual = (unsigned long) MSM_WEB_BASE,
		.pfn = __phys_to_pfn(MSM_WEB_PHYS),
		.length = MSM_WEB_SIZE,
		.type = MT_DEVICE_NONSHARED,
	},
};

static void __init eve_map_io(void)
{
	msm_map_common_io();
	iotable_init(eve_io_desc, ARRAY_SIZE(eve_io_desc));
	msm_clock_init(msm_clocks_7x01a, msm_num_clocks_7x01a);
}

MACHINE_START(EVE, "eve")
	.boot_params	= 0x10000100,
	.fixup		= eve_fixup,
	.map_io		= eve_map_io,
	.init_irq	= msm_init_irq,
	.init_machine	= eve_init,
	.timer		= &msm_timer,
MACHINE_END
