/* drivers/video/msm/src/panel/mddi/mddi_innotek.c
 *
 * Copyright (C) 2008 QUALCOMM Incorporated.
 * Copyright (c) 2008 QUALCOMM USA, INC.
 *
 * All source code in this file is licensed under the following license
 * except where indicated.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <mach/vreg.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/msm_fb.h>
#include <linux/io.h>
#include <asm/mach-types.h>
#include "proc_comm.h"
#include "devices.h"

#include "board-eve.h"

#define PANEL_DEBUG 0

static struct vreg *vreg_gp1;
static struct vreg *vreg_gp2;

/* The comment from AMSS codes:
 * Dot clock (10MHz) / pixels per row (320) = rows_per_second
 * Rows Per second, this number arrived upon empirically
 * after observing the timing of Vsync pulses
 * XXX: TODO: change this values for INNOTEK PANEL */
// static uint32_t mddi_innotek_rows_per_second = 31250;
// static uint32_t mddi_innotek_rows_per_refresh = 480;
// static uint32_t mddi_innotek_usecs_per_refresh = 15360; /* rows_per_refresh / rows_per_second */

struct display_table {
	unsigned reg;
	unsigned char count;
	unsigned char val_list[16];
};

#define REGFLAG_DELAY             0XFFFE
#define REGFLAG_END_OF_TABLE      0xFFFF   // END OF REGISTERS MARKER

/**
 * The count must be a multiple of four bytes, and the val_list
 * can be padded by zeros to archive this
 */
static struct display_table mddi_innotek_display_on[] = {

	// Display on sequence
	{0x29, 4, {0x00, 0x00, 0x00, 0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_display_off[] = {

	// Display off sequence
	{0x28, 4, {0x00, 0x00, 0x00, 0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_sleep_mode_on_data[] = {

	/* Display Off Sequence */
	{0x28, 4, {0x00, 0x00, 0x00, 0x00}},

	/* Sleep Mode in Sequence */
	{0x10, 4, {0x00, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, 120, {}},

	/* MDDI Client Sleep */
	{0xE0, 4, {0x02, 0x00, 0x00, 0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_power_on[] = {

	/* Power ON Sequence */
	{0x11, 4, {0x00, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0xF3, 12, {0x01, 0x01, 0x00, 0x00, 0x07, 0x44, 0x20, 0x7F, 0x00, 0x00, 0x00, 0x00}},
	{0xF4, 8, {0x0D, 0x23, 0x31, 0x55, 0x11, 0x00, 0x00, 0x00}},
	{0xF5, 8, {0x10, 0x00, 0x08, 0xF0, 0x35, 0x1F, 0x00, 0x00}},

	// Normal Mode
	{0x13, 4, {0x00, 0x00, 0x00, 0x00}},

	// Column Address
	{0x2A, 4, {0x00, 0x00, 0x01, 0x3F}},

	// Page Address
	{0x2B, 4, {0x00, 0x00, 0x01, 0xDF}},

	// Memory Data Address Control
	{0x36, 4, {0x48, 0x00, 0x00, 0x00}},

	// Interface Pixel Format
	{0x3A, 4, {0x05, 0x00, 0x00, 0x00}},

	// Display Control Set
	{0xF2, 12, {0x16, 0x14, 0x03, 0x03, 0x03, 0x08, 0x08, 0x03, 0x00, 0x15, 0x15, 0x00}},

	// Tearing Effect Lion On
	{0x35, 4, {0x00, 0x00, 0x00, 0x00}},

	// CABC Control
	{0x51, 4, {0xFF, 0x00, 0x00, 0x00}},
	{0x53, 4, {0x2C, 0x00, 0x00, 0x00}},
	{0x55, 4, {0x03, 0x00, 0x00, 0x00}},
	{0x5E, 4, {0x00, 0x00, 0x00, 0x00}},
	{0xCA, 4, {0x80, 0x80, 0x3F, 0x00}},
	{0xCB, 4, {0x03, 0x00, 0x00, 0x00}},
	{0xCC, 8, {0x20, 0x01, 0x8F, 0x00, 0xEF, 0x00, 0x00, 0x00}},
	{0xCD, 4, {0x04, 0x97, 0x00, 0x00}},

	// Positive Gamma Red
	{0xF7, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Negative Gamma Red
	{0xF8, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Positive Gamma Green
	{0xF9, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Negative Gamma Green
	{0xFA, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Positive Gamma Blue
	{0xFB, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Negative Gamma Blue
	{0xFC, 16, {0x00, 0x00, 0x00, 0x11, 0x18, 0x23, 0x2A, 0x29, 0x12, 0x08, 0x05, 0x00, 0x00, 0x22, 0x22, 0x00}},

	// Gate Control Register
	{0xFD, 4, {0x11, 0x3B, 0x00, 0x00}},

	// GRAM Write
	{0x2C, 4, {0x00, 0x00, 0x00, 0x00}},

	// Dispaly On Sequence
	{0x29, 4, {0x00, 0x00, 0x00, 0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-25, prevent the LCD shift (referenced from swift) */
static struct display_table mddi_innotek_position[] = {
	// Column Address
	{0x2A, 4, {0x00, 0x00, 0x01, 0x3F}},
	// Page Address
	{0x2B, 4, {0x00, 0x00, 0x01, 0xDF}},
};

static void display_table(struct msm_mddi_client_data *client_data, struct display_table *table, unsigned int count)
{
	unsigned int i;

	for(i = 0; i < count; i++) {

		unsigned reg;
		reg = table[i].reg;

		switch (reg) {

			case REGFLAG_DELAY :
				msleep(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				client_data->remote_write_vals(client_data, table[i].val_list, reg, table[i].count);
		}
	}

}

#if 0
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-25, prevent the LCD shift (referenced from swift) */
void mddi_innoteck_fix_position(void)
{
	display_table(mddi_innotek_position, sizeof(mddi_innotek_power_on)/sizeof(struct display_table));
}
EXPORT_SYMBOL(mddi_innoteck_fix_position);
#endif

static void mddi_innotek_lcd_panel_reset(void)
{
	udelay(15);

	gpio_set_value(GPIO_LCD_RESET_N, 1);
	mdelay(10);

	gpio_set_value(GPIO_LCD_RESET_N, 0);
	mdelay(10);

	gpio_set_value(GPIO_LCD_RESET_N, 1);
	mdelay(10);
}

static int innotek_client_init(
		struct msm_mddi_bridge_platform_data *bridge_data,
		struct msm_mddi_client_data *client_data)
{
	printk("%s: entered\n",__func__);

	client_data->auto_hibernate(client_data, 0);
	display_table(client_data, mddi_innotek_power_on, ARRAY_SIZE(mddi_innotek_power_on));
	client_data->auto_hibernate(client_data, 1);
	return 0;
}

static int innotek_client_uninit(
		struct msm_mddi_bridge_platform_data *bridge_data,
		struct msm_mddi_client_data *cdata)
{
	printk("%s: entered - stub\n",__func__);
	return 0;
}

static int innotek_panel_unblank(
		struct msm_mddi_bridge_platform_data *bridge_data,
		struct msm_mddi_client_data *client_data)
{
	int ret = 0;

	printk("%s: entered - stub\n",__func__);
	//display_table(client_data, mddi_innotek_display_on, ARRAY_SIZE(mddi_innotek_display_on));
	return ret;
}

static int innotek_panel_blank(
		struct msm_mddi_bridge_platform_data *bridge_data,
		struct msm_mddi_client_data *client_data)
{
	int ret = 0;
	printk("%s: entered - stub\n",__func__);
	return ret;
}
static void eve_mddi_power_client(struct msm_mddi_client_data *client_data,
		int on)
{
	printk("%s(%d): entered\n",__func__,on);

	if(on) {
		vreg_enable(vreg_gp1);
		vreg_enable(vreg_gp2);
		mddi_innotek_lcd_panel_reset();
	} else {
		 vreg_disable(vreg_gp1);
         vreg_disable(vreg_gp2);
	}
}

#define NT35399_MFR_NAME    0x0bda
#define NT35399_PRODUCT_CODE    0x8a47

static void innotek_fixup(uint16_t * mfr_name, uint16_t * product_code)
{
	*mfr_name = NT35399_MFR_NAME ;
	*product_code= NT35399_PRODUCT_CODE ;
}

static struct resource resources_msm_fb[] = {
	{
		.start  = MSM_FB_BASE,
		.end    = MSM_FB_BASE + MSM_FB_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	}
};

static struct msm_mddi_bridge_platform_data innotek_client_data = {

	.init = innotek_client_init,
	.uninit = innotek_client_uninit,
	.blank = innotek_panel_blank,
	.unblank = innotek_panel_unblank,
	.fb_data = {
		.xres = 320,
		.yres = 480,
		.output_format = 0,
	},
};

static struct msm_mddi_platform_data mddi_pdata = {
	.clk_rate = 81920000,
	.power_client = eve_mddi_power_client,
	.fixup = innotek_fixup,
	.fb_resource = resources_msm_fb,
	.num_clients = 1,
	.client_platform_data = {
		{
			.product_id =
				(NT35399_MFR_NAME << 16 | NT35399_PRODUCT_CODE),
			.name = "mddi_c_simple" ,
			.id = 0,
			.client_data = &innotek_client_data,
		},
	},
};

int __init eve_init_panel(void)
{
	int rc = -1;

	if (!machine_is_eve())
		return 0;

	vreg_gp1 = vreg_get(0, "gp1");
	if (IS_ERR(vreg_gp1))
		return PTR_ERR(vreg_gp1);
	vreg_gp2 = vreg_get(0, "gp2");
	if (IS_ERR(vreg_gp2))
		return PTR_ERR(vreg_gp2);

	gpio_tlmm_config(GPIO_CFG(GPIO_LCD_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	rc = platform_device_register(&msm_device_mdp);
	if (rc)
		return rc;
	msm_device_mddi0.dev.platform_data = &mddi_pdata;
	rc = platform_device_register(&msm_device_mddi0);
	if (rc) {
		printk("platform_device_register returned %d\n",rc);
		return rc;
	}

	return 0;
}

device_initcall(eve_init_panel);

