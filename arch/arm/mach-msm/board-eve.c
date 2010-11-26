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
	.no_allocator = 0,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_camera_pdata = {
    .name = "pmem_camera",
    .cached = 1,
    .start = MSM_PMEM_CAMERA_BASE,
    .size = MSM_PMEM_CAMERA_SIZE,
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

static struct platform_device android_pmem_camera_device = {
    .name = "android_pmem",
    .id = 2,
    .dev = { .platform_data = &android_pmem_camera_pdata },
};

static struct platform_device android_pmem_adsp_device = {
    .name = "android_pmem",
    .id = 1,
    .dev = { .platform_data = &android_pmem_adsp_pdata },
};

/* usb */
static int eve_phy_init_seq[] = { 0x1D, 0x0D, 0x1D, 0x10, -1 };

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.phy_init_seq = eve_phy_init_seq,
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns = 1,
	.vendor = "Qualcomm",
	.product = "Halibut",
	.release = 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name = "usb_mass_storage",
	.id = -1,
	.dev = {
		.platform_data = &mass_storage_pdata,
	},
};

static char *usb_functions[] = { "usb_mass_storage" };
static char *usb_functions_adb[] = { "usb_mass_storage", "adb" };
static char *usb_functions_adb_diag[] = { "usb_mass_storage", "adb", "diag" };

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x0c01,
		.num_functions	= ARRAY_SIZE(usb_functions),
		.functions	= usb_functions,
	},
	{
		.product_id	= 0x0c02,
		.num_functions	= ARRAY_SIZE(usb_functions_adb),
		.functions	= usb_functions_adb,
	},
    {
        .product_id = 0x0c07,
        .num_functions  = ARRAY_SIZE(usb_functions_adb_diag),
        .functions  = usb_functions_adb_diag,
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
	.num_functions = ARRAY_SIZE(usb_functions_adb),
	.functions = usb_functions_adb,
};

static struct platform_device android_usb_device = {
	.name = "android_usb",
	.id = -1,
	.dev = {
		.platform_data = &android_usb_pdata,
	},
};

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
	&msm_device_uart3,
	&msm_device_smd,
	&msm_device_nand,

	&msm_device_hsusb,
	&usb_mass_storage_device,
	&android_usb_device,
	&eve_battery_device,

	&msm_device_i2c,
	&msm_device_touchscreen,
	&eve_bl_i2c_adap_bus,

	&eve_compass_i2c_bus,
	&eve_prox_i2c_bus,
	&eve_accel_i2c_bus,

	&eve_touch_i2c_bus,
	&eve_qwerty_device,
	&pwrkey_device,

	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_camera_device,
	&hw3d_device,
	&eve_snd,
};

extern struct sys_timer msm_timer;

static struct msm_acpu_clock_platform_data eve_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.power_collapse_khz = 19200000,
	.wait_for_irq_khz = 128000000,
};

static void __init eve_init(void)
{
	msm_device_hsusb.dev.platform_data = &msm_hsusb_pdata;
	msm_acpu_clock_init(&eve_clock_data);

	/* register drivers for the i2c busses */
	/* each pair of SCL and SDA lines is one bus */
	i2c_register_board_info(I2C_BUS_NUM_MOTION, &accel_i2c_bdinfo, 1);
	i2c_register_board_info(I2C_BUS_NUM_BACKLIGHT, &bl_i2c_device, 1);
	i2c_register_board_info(I2C_BUS_NUM_TOUCH, &i2c_board_touch, 1);
	i2c_register_board_info(I2C_BUS_NUM_PROX, &prox_i2c_bdinfo, 1);
	i2c_register_board_info(I2C_BUS_NUM_COMPASS, &compass_i2c_bdinfo, 1);

	gpio_request(GPIO_COMPASS_RESET, "compass_rst");
	gpio_direction_output(GPIO_COMPASS_RESET, 1);
	gpio_request(GPIO_COMPASS_IRQ, "compass_int");
	gpio_direction_input(GPIO_COMPASS_IRQ);

	platform_add_devices(devices, ARRAY_SIZE(devices));

	msm_hsusb_set_vbus_state(1);
}

static void __init eve_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].node = PHYS_TO_NID(PHYS_OFFSET);
	mi->bank[0].size = MSM_LINUX_SIZE;
}

static void __init eve_map_io(void)
{
	msm_map_common_io();
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
