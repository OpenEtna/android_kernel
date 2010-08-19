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

/* LGE_CHANGE_S [cleaneye@lge.com] 2009-04-01, eve keypad platform device information */
#if 0 /*CONFIG_MACH_ADAM*/
static unsigned int keypad_row_gpios_eve[] = {
	36, 37, 38, 39, 40, 41, 42,18, 20
};

static unsigned int keypad_col_gpios_eve[] = { 31, 32 };
#define KEYMAP_INDEX_EVE(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios_eve) + (row))
#define NUM_EXTRA_KEY   6
#endif /* CONFIG_MACH_ADAM */
/* LGE_CHANGE_E [cleaneye@lge.com] 2009-04-01, eve keypad platform device information */

/* LGE_CHANGE_S [diyu@lge.com] 2009-05-18, eve rev_b gpio-key(side-key)  */
#if defined(CONFIG_MACH_EVE) //REV_B_SIDE_GPIO_KEY(VolumeUp/Down)

static unsigned int keypad_row_gpios_eve[] = { 40 };
static unsigned int keypad_col_gpios_eve[] = { 31, 32 };
#define KEYMAP_INDEX_EVE(col, row) ((row)*ARRAY_SIZE(keypad_row_gpios_eve) + (col))
static const unsigned short keypad_keymap_eve[ARRAY_SIZE(keypad_col_gpios_eve) *
									ARRAY_SIZE(keypad_row_gpios_eve) ] = {
	[KEYMAP_INDEX_EVE(0, 0)] = KEY_VOLUMEDOWN, /* UP KEY_ROW[0]*/
	[KEYMAP_INDEX_EVE(0, 1)] = KEY_VOLUMEUP,/* UP_R KEY_ROW[1]*/
};
#endif
/* LGE_CHANGE_E [diyu@lge.com] 2009-05-18, eve rev_b gpio-key(side-key)  */


static const unsigned short keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

/* LGE_CHANGE_S [cleaneye@lge.com] 2009-04-01, eve keypad platform device information */
#if defined(CONFIG_MACH_EVE)

int gpio_event_matrix_func(struct gpio_event_input_devs *input_devs,
	struct gpio_event_info *info, void **data, int func);

static struct gpio_event_matrix_info eve_keypad_matrix_info = {
	//.info.func	= keypad_gpio_event_matrix_func,
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_eve,
	.output_gpios	= keypad_col_gpios_eve,
	.input_gpios	= keypad_row_gpios_eve,
	.noutputs	= ARRAY_SIZE(keypad_col_gpios_eve),
	.ninputs	= ARRAY_SIZE(keypad_row_gpios_eve),
	.settle_time.tv.nsec = 0,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS
};

int eve_keypad_power(const struct gpio_event_platform_data *pdata, bool on)
{
	return 0;
}

static struct gpio_event_info *eve_keypad_info[] = {
	&eve_keypad_matrix_info.info
};

static struct gpio_event_platform_data eve_keypad_data = {
	.name		= "eve_keypad",
	.info		= eve_keypad_info,
	.info_count	= ARRAY_SIZE(eve_keypad_info),
	.power = eve_keypad_power,
};

struct platform_device keypad_device_eve = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &eve_keypad_data,
	},
};
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [cleaneye@lge.com] 2009-04-01, eve keypad platform device information */

static struct input_dev *keypad_dev;

static int keypad_gpio_event_matrix_func(struct input_dev *input_dev,
					  struct gpio_event_info *info,
					  void **data, int func)
{
	int err;
	int i;

	err = gpio_event_matrix_func(input_dev, info, data, func);

	if (func == GPIO_EVENT_FUNC_INIT && !err) {
		keypad_dev = input_dev;
		for (i = 0; i < ARRAY_SIZE(keypad_virtual_keys); i++)
			set_bit(keypad_virtual_keys[i] & KEY_MAX,
				input_dev->keybit);
	} else if (func == GPIO_EVENT_FUNC_UNINIT) {
		keypad_dev = NULL;
	}

	return err;
}

struct input_dev *msm_keypad_get_input_dev(void)
{
	return keypad_dev;
}

