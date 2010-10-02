/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>

#define GPIO_HALL_IC_IRQ    18

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-26, not used */
#if 0
/* don't turn this on without updating the ffa support */
#define SCAN_FUNCTION_KEYS 0

#define CONFIG_MACH_EVE_REV_A 0

static unsigned int keypad_row_gpios[] = {
	31, 32, 33, 34, 35, 41
#if SCAN_FUNCTION_KEYS
	, 42
#endif
};

static unsigned int keypad_col_gpios[] = { 36, 37, 38, 39, 40 };
#endif

static unsigned int keypad_row_gpios_eve[] = { 40 };
static unsigned int keypad_col_gpios_eve[] = { 31, 32 };
#define KEYMAP_INDEX_EVE(col, row) ((row)*ARRAY_SIZE(keypad_row_gpios_eve) + (col))
static const unsigned short keypad_keymap_eve[ARRAY_SIZE(keypad_col_gpios_eve) *
									ARRAY_SIZE(keypad_row_gpios_eve) ] = {
	[KEYMAP_INDEX_EVE(0, 0)] = KEY_VOLUMEDOWN, /* UP KEY_ROW[0]*/
	[KEYMAP_INDEX_EVE(0, 1)] = KEY_VOLUMEUP,/* UP_R KEY_ROW[1]*/
};


static const unsigned short keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

static struct gpio_event_matrix_info eve_keypad_matrix_info = {
    .info.func	= gpio_event_matrix_func,
    .keymap		= keypad_keymap_eve,
    .output_gpios	= keypad_col_gpios_eve,
    .input_gpios	= keypad_row_gpios_eve,
    .noutputs	= ARRAY_SIZE(keypad_col_gpios_eve),
    .ninputs	= ARRAY_SIZE(keypad_row_gpios_eve),
    .settle_time.tv.nsec = 0,
    .poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
    .flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
                  GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_PRINT_MAPPED_KEYS
};

static struct gpio_event_direct_entry eve_keypad_switch_map[] = {
    { GPIO_HALL_IC_IRQ,       SW_LID       }
};

static struct gpio_event_input_info eve_keypad_switch_info = {
    .info.func = gpio_event_input_func,
    .flags = 0,
    .type = EV_SW,
    .keymap = eve_keypad_switch_map,
    .keymap_size = ARRAY_SIZE(eve_keypad_switch_map)
};

static struct gpio_event_info *eve_keypad_info[] = {
    &eve_keypad_matrix_info.info,
    &eve_keypad_switch_info.info
};

static struct gpio_event_platform_data eve_keypad_data = {
    .name = "eve-keypad",
    .info = eve_keypad_info,
    .info_count = ARRAY_SIZE(eve_keypad_info)
};

static struct platform_device eve_keypad_device = {
    .name = GPIO_EVENT_DEV_NAME,
    .id = 0,
    .dev        = {
        .platform_data  = &eve_keypad_data,
    },
};

static int __init eve_init_keypad(void)
{
    //if (!machine_is_trout())
    //    return 0;

    return platform_device_register(&eve_keypad_device);
}

device_initcall(eve_init_keypad);

